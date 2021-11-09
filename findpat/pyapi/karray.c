#include <Python.h>
#include <sys/mman.h>

#include "structmember.h"
#include "karray.h"
#include "kmacros.h"


#include "macros.h"

#define debug_call(METH, self) debug("--- PyKArray: %9s %p --> n=%4ld [%d] buf=%p bind=%p refcount=%u\n", METH, (self), (self)->n, (self)->fldsz, (self)->buf, (self)->bind_to, (unsigned int)(self)->ob_refcnt)

/* Errores de Python */
static PyObject *KapowArrayError;

/* Tipo PyKArray */

PyTypeObject PyKArray_Type;

/* Constructors / Destructors */

PyObject *
PyKArray_new(void) {
	PyKArray *self;

	self = (PyKArray *)(&PyKArray_Type)->tp_alloc(&PyKArray_Type, 0);
	if (self != NULL) {
		self->fldsz = 0;
		self->n = 0;
		self->buf = NULL;
		self->bind_to = NULL;
		self->binded_from = 0;
	}

	debug_call("NEW", self);

	return (PyObject *)self;
}

static PyObject *
karray_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyKArray *self;

	self = (PyKArray*)PyKArray_new();

	return (PyObject *)self;
}

int
PyKArray_init(PyKArray *self, unsigned long n, unsigned int fldsz) {
	int res = 0;
	self->n = n;
	self->fldsz = fldsz;
	self->buf = NULL;
	self->bind_to = NULL;
	self->binded_from = 0;
	if (n) {
		self->buf = malloc(n*(unsigned long)fldsz);
		if (!self->buf) {
			res = -1;
			PyErr_Format(KapowArrayError, "No enough memory to alloc %lu bytes in resize()", n*(unsigned long)fldsz);
		}
	}

	debug_call("INIT", self);
	return res;
}

static int
karray_init(PyKArray *self, PyObject *args, PyObject *kwds) {
	unsigned int sz=0;
	unsigned long n=0;

	static char *kwlist[] = {"n", "fldsz", NULL};

	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|kI", kwlist, &n, &sz))
		return -1;

	if (sz == 3 || (sz > 4 && sz != 8)) { /* 0,1,2,4,8 */
		PyErr_Format(KapowArrayError, "fldsz must be 0, 1, 2, 4 or 8.");
		return -1;
	} 

	if (n && !sz) sz = 1;

	PyKArray_init(self, n, sz);

	return 0;
}


static void
_karray_cleanbind(PyKArray *self) {
	if (PyKArray_Check(self->bind_to)) ((PyKArray*)(self->bind_to))->binded_from--;
	else if (PyFile_Check(self->bind_to)) {
		munmap(self->buf, self->n);
		// PyFile_DecUseCount((PyFileObject*)self->bind_to);
	}
	Py_CLEAR(self->bind_to);
}

static int
PyKArray_clear(PyKArray* self) {
	debug_call("CLEAR", self);

	if (self->bind_to) {
		_karray_cleanbind(self);
		self->buf = NULL;
		self->n = 0;
	} else {
		if (self->buf) {
			free(self->buf);
			self->buf = 0;
			self->n = 0;
		}
	}
	return 0;
}

static void
PyKArray_dealloc(PyKArray* self) {
	debug_call("DEALLOC", self);
	PyKArray_clear(self);
	self->ob_type->tp_free((PyObject*)self);
}


/* Getters y setters */

#define MAX_REPR_LEN 1024
#define MAX_REPR_ELEMS 128
static PyObject *
PyKArray_str(PyKArray *self) {
	PyObject *res;
	char rep[MAX_REPR_LEN];
	int i, lrep=0;
	unsigned long x, msk;
	unsigned char* p;

	if (self->fldsz == 1) {
		res = PyString_FromStringAndSize(self->buf, self->n>0x7fffffffL?0x7fffffffL:self->n);
	} else {
		rep[0] = '['; lrep = 1;
		p = self->buf;
		msk = (self->fldsz<sizeof(long) ? (1L << (long)(self->fldsz * 8)) : 0) - 1;
		//debug("** p=%p  msk=%lx\n", p, msk);
		for (i=0; i<self->n && i < MAX_REPR_ELEMS && lrep < MAX_REPR_LEN-5; ++i) {
			x = *((long*)p) & msk; // FIXME: Esto se puede ir del buffer en el último elemento
			if (i+1 < self->n) {
				lrep += snprintf(rep+lrep, MAX_REPR_LEN-lrep, "%lu, ", x);
			} else {
				lrep += snprintf(rep+lrep, MAX_REPR_LEN-lrep, "%lu]", x);
			}
			p += self->fldsz;
		}
		if (lrep >= MAX_REPR_LEN-5) {
			strncpy(rep+MAX_REPR_LEN-5, "...]", 5);
		} else {
			if (i < self->n) {
				lrep += snprintf(rep+lrep, MAX_REPR_LEN-lrep, "...]");
			}
		}
		res = PyString_FromString(rep);
	}

	return res;
}

static PyObject *
PyKArray_repr(PyKArray *self) {
	PyObject *res;
	res = PyString_FromFormat("<%s(n=%lu, fldsz=%u, refs=%zd) object at %p>", self->ob_type->tp_name, self->n, self->fldsz, self->ob_refcnt, self);
	return res;
}


/* Methods */

PyDoc_STRVAR(memset__doc__,
"A.memset([byte]) -> A\n"
"\n"
"Fills the array with the given byte (default is zero) regardless\n"
"the size of each element. Returns a reference to the same object.\n");

static PyObject *
karray_memset(PyKArray *self, PyObject *args) {
	unsigned char vl = 0;

	if (!PyArg_ParseTuple(args, "|b:memset", &vl))
		return NULL;

	debug("memset(%p, %u, %lu);\n", self->buf, vl, self->n * self->fldsz);
	Py_BEGIN_ALLOW_THREADS
	memset(self->buf, vl, self->n * self->fldsz);
	Py_END_ALLOW_THREADS

	Py_INCREF(self); // Devuelvo una nueva referencia al propio objeto
	return (PyObject *)self;
}


PyDoc_STRVAR(fill__doc__,
"A.fill([element]) -> A\n"
"\n"
"Fills the array with the given element (default is zero).\n"
"Returns a reference to the same object.\n");

static PyObject *
karray_fill(PyKArray *self, PyObject *args) {
	PyObject *res = (PyObject *)self;
	unsigned PY_LONG_LONG vl = 0;
	unsigned long i;

	if (!PyArg_ParseTuple(args, "|K:fill", &vl))
		return NULL;

	unsigned int sz = self->fldsz;

	Py_BEGIN_ALLOW_THREADS
	if (sz == 1) {
		memset(self->buf, vl, self->n);
	} else if (sz == 2) {
		unsigned short* p = self->buf;
		for(i=self->n;i;i--) *(p++) = vl;
	} else if (sz == 4) {
		unsigned int* p = self->buf;
		for(i=self->n;i;i--) *(p++) = vl;
	} else if (sz == 8) {
		unsigned PY_LONG_LONG* p = self->buf;
		for(i=self->n;i;i--) *(p++) = vl;
	} else {
		res = NULL;
	}
	Py_END_ALLOW_THREADS

	if (!res) {
		PyErr_Format(KapowArrayError, "Unsupported field size (%u) for fill()", sz);
		return res;
	}

	Py_INCREF(self); // Devuelvo una nueva referencia al propio objeto
	return (PyObject *)self;
}


PyDoc_STRVAR(bindFrom__doc__,
"A.bindFrom(object)\n"
"\n"
"Binds the array A to the given object as an array of char. object could\n"
"be a string.\n");

int
PyKArray_bindFrom(PyKArray *self, PyObject* obj) {
	if (self->bind_to) {
		PyErr_Format(KapowArrayError, "Cannot bind to a %.200s an already binded array. unbind() or clean() it first.", Py_TYPE(obj)->tp_name);
		return -1;
	}
	if (self->binded_from) {
		PyErr_Format(KapowArrayError, "Cannot bind to a %.200s an array binded from another array. unbind() or delete the other array first.", Py_TYPE(obj)->tp_name);
		return -1;
	}

	void* buf;
	unsigned long n;

	if (PyString_Check(obj)) {
		n = PyString_GET_SIZE(obj);
		buf = PyString_AS_STRING(obj);
	} else if (PyFile_Check(obj)) {
		/* Call mmap on the whole file */
		// PyFile_IncUseCount((PyFileObject*)obj);
		FILE* fp = PyFile_AsFile(obj);
		struct stat fst;
		fstat(fileno(fp), &fst);
		buf = mmap(NULL, fst.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileno(fp), 0);
		if (buf == MAP_FAILED) {
			PyErr_Format(KapowArrayError, "Error trying to mmap() to the given file.");
			return -1;
		}
		n = fst.st_size;
	} else {
		PyErr_Format(KapowArrayError, "Don't know how to bind to a %.200s.", Py_TYPE(obj)->tp_name);
		return -1;
	}

	if (self->n) {
		/* Esta llamada no falla */
		if (PyKArray_resize(self, 0, 1) < 0)
			return -1;
	}
	
	Py_INCREF(obj);
	return PyKArray_bindto(self, obj, buf, n, 1);
}

static PyObject *
karray_bindFrom(PyKArray *self, PyObject *args) {
	PyObject* obj;

	if (!PyArg_ParseTuple(args, "O:bindFrom", &obj))
		return NULL;

	if (PyKArray_bindFrom(self, obj) < 0) {
		return NULL;
	}

	Py_RETURN_NONE;
}


PyDoc_STRVAR(resize__doc__,
"A.resize(new_size, [new_field_size]) -> A\n"
"\n"
"Resizes the array and copy the elements to the new array.\n"
"Returns a reference to the same object.\n");

int
PyKArray_resize(PyKArray *self, unsigned long nn, unsigned int ssz) {
	if (self->bind_to) {
		PyErr_Format(KapowArrayError, "Cannot resize a binded array. unbind() it first.");
		return -1;
	}
	if (self->binded_from) {
		PyErr_Format(KapowArrayError, "Cannot resize an array binded to another array. unbind() or delete the other array first.");
		return -1;
	}

	if (!ssz && nn) { ssz = self->fldsz; if (!ssz) ssz = 1; }
	if (!nn && !self->n) { self->fldsz = ssz; return 0; }
	if (nn == self->n && ssz == self->fldsz) return 0; // Don't resize if useless

	unsigned PY_LONG_LONG cpy = self->n;
	if (nn < cpy) cpy = nn; // min(self->n, nn) == elems to copy
	unsigned long nsz = nn * ssz;
	unsigned long csz = self->n * self->fldsz;
	void* nbuf = NULL;
	/* Get new memory if needed */
	if (nsz != csz && nsz) {
		nbuf = malloc(nsz);
		if (!nbuf) {
			PyErr_Format(KapowArrayError, "No enough memory to alloc %lu bytes in resize()", (unsigned long)nsz);
			return -1;
		}
	} else if (nsz == csz) nbuf = self->buf;
	/* Copy the memory or the elements, if needed */
#define __set_item(i, vl) (\
	ssz==1?((unsigned char*)nbuf)[i] = (vl): \
	ssz==2?((unsigned short*)nbuf)[i] = (vl): \
	ssz==4?((unsigned int*)nbuf)[i] = (vl): \
	ssz==8?((unsigned PY_LONG_LONG*)nbuf)[i] = (vl): \
	0)
	if (cpy) {
		if (self->fldsz == ssz) {
			cpy *= PyKArray_GET_ITEMSZ(self);
			memcpy(nbuf, self->buf, cpy);
		} else if (self->fldsz < ssz) { /* Copy elements from the end to the begin */
			unsigned long i;
			for (i = cpy-1; i != -1UL; i--) {
				const unsigned PY_LONG_LONG elem = PyKArray_GET_ITEM(self, i);
				__set_item(i, elem);
			}
		} else { /* (self->fldsz > ssz) --  Copy elements from the begin to the end */
			unsigned long i;
			unsigned PY_LONG_LONG msk = (1ULL << ((unsigned PY_LONG_LONG)(ssz)*8))-1ULL;
			for (i = 0; i<cpy; i++) {
				unsigned PY_LONG_LONG elem = PyKArray_GET_ITEM(self, i);
				if (elem > msk) elem = msk;
				__set_item(i, elem);
			}
		}
	}
#undef __set_item
	/* Free old memory, if needed */
	if (nsz != csz && self->n) {
		free(self->buf);
	}

	self->n = nn;
	self->fldsz = ssz;
	self->buf = nbuf;
	debug_call("RESIZE", self);
	return 0;
}

static PyObject *
karray_resize(PyKArray *self, PyObject *args) {
	unsigned long nn;
	unsigned int sz = self->fldsz;

	if (!PyArg_ParseTuple(args, "k|I:resize", &nn, &sz))
		return NULL;

	if (PyKArray_resize(self, nn, sz) < 0) {
		return NULL;
	}

	Py_INCREF(self); // Devuelvo una nueva referencia al propio objeto
	return (PyObject *)self;
}


PyDoc_STRVAR(clean__doc__,
"A.clean()\n"
"\n"
"Cleans the current array setting its size to 0.\n");
static PyObject *
karray_clean(PyKArray *self) {
	if (self->binded_from) {
		PyErr_Format(KapowArrayError, "Cannot resize an array binded to another array. unbind() or delete the other array first.");
		return NULL;
	}

	if (self->bind_to) {
		_karray_cleanbind(self);
		self->buf = NULL;
	}

	if (self->buf) free(self->buf);
	self->buf = NULL;
	self->n = 0;

	Py_RETURN_NONE;
}


PyDoc_STRVAR(inverse__doc__,
"A.inverse(A2=None) -> A2\n"
"\n"
"Computes the inverse permutation of A in A2 and returns it. If A2 is \n"
"not given a new one will be created. A2 must have the same size that A.\n");

int
PyKArray_inverse(PyKArray *self, PyKArray *other) {
	int res = 0;
	if (other->n && !(self->n == other->n && self->fldsz == other->fldsz)) {
		PyErr_Format(KapowArrayError, "The destination array must have the same size.");
		return -1;
	}

	if (self->n != other->n || self->fldsz != other->fldsz) {
		if (PyKArray_resize(other, self->n, self->fldsz) < 0)
			return -1;
	}

	unsigned long i, n = self->n;
	#define __inverse(type) type *r = other->buf, *p = self->buf; for (i=0;i<n;i++) r[*(p++)] = i;

	Py_BEGIN_ALLOW_THREADS

	if (self->fldsz == 1) { __inverse(unsigned char) }
	else if (self->fldsz == 2) { __inverse(unsigned short) }
	else if (self->fldsz == 4) { __inverse(unsigned int) }
	else if (self->fldsz == 8) { __inverse(unsigned long long) }
	else {
		if (self->n) {
			res = -1;
		}
	}
	Py_END_ALLOW_THREADS

	if (res < 0) {
		PyErr_Format(KapowArrayError, "Cannot inverse such array.");
		return res;
	}
	
	debug_call("INVERSE", self);
	return 0;
}

static PyObject *
karray_inverse(PyKArray *self, PyObject *args) {
	PyKArray *other = NULL;

	if (!PyArg_ParseTuple(args, "|O:inverse", &other))
		return NULL;

	if (other) {
		Py_INCREF(other);
	} else {
		other = (PyKArray*)PyKArray_new();
	}

	if (PyKArray_inverse(self, other) < 0) {
		Py_DECREF(other);
		return NULL;
	}

	return (PyObject *)other;
}


PyDoc_STRVAR(reverse__doc__,
"A.reverse() -> None\n"
"\n"
"Computes the reverse of A in the same array A.\n");

int
PyKArray_reverse(PyKArray *self) {
	int res = 0;

	unsigned long i, n = self->n;
	#define __reverse(type) type *p = self->buf; type *q = p+n; for (i=n/2;i;i--) { type _tmp = *p; *(p++) = *(--q); *q = _tmp; }

	Py_BEGIN_ALLOW_THREADS

	if (self->fldsz == 1) { __reverse(unsigned char) }
	else if (self->fldsz == 2) { __reverse(unsigned short) }
	else if (self->fldsz == 4) { __reverse(unsigned int) }
	else if (self->fldsz == 8) { __reverse(unsigned long long) }
	else {
		if (self->n) {
			res = -1;
		}
	}
	Py_END_ALLOW_THREADS
	
	#undef __reverse

	if (res < 0) {
		PyErr_Format(KapowArrayError, "Cannot reverse such array.");
		return res;
	}
	
	debug_call("REVERSE", self);
	return 0;
}

static PyObject *
karray_reverse(PyKArray *self) {
	if (PyKArray_reverse(self) < 0) {
		return NULL;
	}
	Py_RETURN_NONE;
}


PyDoc_STRVAR(bind__doc__,
"A.bind(n=current_n, fldsz=current_fldsz, start=0) -> A2\n"
"\n"
"Builds a new Array A2 binded to a porton of the original Array. Note\n"
"that the original Array will not be deleted until the new one was\n"
"deleted, due the binded memory.\n");

PyAPI_FUNC(int)
PyKArray_bindto(PyKArray *self, PyObject *obj, void* buf, unsigned long n, unsigned int fldsz) {
	if (self->bind_to || self->binded_from) {
		PyErr_SetString(KapowArrayError, "Array already binded.");
		return -1;
	}
	if (self->buf) {
		pz_free(self->buf);
	}
	self->buf = buf;
	self->n = n;
	self->fldsz = fldsz;
	
	self->bind_to = obj;
	if (PyKArray_Check(self->bind_to)) ((PyKArray*)(self->bind_to))->binded_from++;
	return 0;
}

PyKArray * PyKArray_bind(PyKArray *self, unsigned long st, unsigned long nn, unsigned int fldsz) {
	// Check de la región dentro del array
	if (st * (long)(self->fldsz) + nn * (long)fldsz > self->n * (long)(self->fldsz)) {
		PyErr_Format(KapowArrayError, "The binded region (start=%lu, len=%lu, fldsz=%u) runs beyond the original region (len=%lu, fldsz=%u).", st, nn, fldsz, self->n, self->fldsz);
		return NULL;
	}

	PyKArray *other = (PyKArray*)PyKArray_new();
	other->n = nn;
	other->fldsz = fldsz;
	other->bind_to = (self->bind_to && !PyFile_Check(self->bind_to))?self->bind_to:(PyObject*)self;
	Py_INCREF(other->bind_to);
	if (PyKArray_Check(other->bind_to)) ((PyKArray*)(other->bind_to))->binded_from++;
	other->buf = ((unsigned char*)self->buf) + st * self->fldsz;
	
	return other;
}

static PyObject *
karray_bind(PyKArray *self, PyObject *args, PyObject *kwds) {
	unsigned long nn = self->n;
	unsigned int fldsz = self->fldsz;
	unsigned long st = 0;

	static char *kwlist[] = {"n", "fldsz", "start", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|kIk:bind", kwlist, &nn, &fldsz, &st))
		return NULL;

	// TODO: Raise error si fldsz == 0;

	return (PyObject *)PyKArray_bind(self, st, nn, fldsz);
}


PyDoc_STRVAR(unbind__doc__,
"A.unbind() -> A\n"
"\n"
"Given an array binded to another, unbinds the array copying the content\n"
"to a new portion of memory. Returns a reference to the same array.\n");

static PyObject *
karray_unbind(PyKArray *self) {

	if (! self->bind_to) {
		PyErr_Format(KapowArrayError, "Cannot unbind an Array not binded to another.\n");
		return NULL;
	}

	void* nbuf = NULL;
	if (self->n) {
		nbuf = malloc(self->n * self->fldsz);
		memcpy(nbuf, self->buf, self->n * (long)self->fldsz);
	}
	self->buf = nbuf;
	_karray_cleanbind(self);

	Py_INCREF(self);
	return (PyObject *)self;
}


PyDoc_STRVAR(store__doc__,
"A.store(f)\n"
"\n"
"Stores the array in the given file f (should be open for write). This\n"
"can be latter loaded with load().\n");

int
PyKArray_store(PyKArray *self, FILE *fp) {
	int result = TRUE;
	char magic[4] = "KA00";
	int uno = 1;
	magic[2] += sizeof(self->n);
	magic[3] += *(char*)&uno;

	result = fwrite(magic, 4, 1, fp) == 1  && result;
	result = fwrite(&self->n, sizeof(self->n), 1, fp) == 1  && result;
	result = fwrite(&self->fldsz, sizeof(self->fldsz), 1, fp) == 1  && result;
	result = fwrite(self->buf, self->fldsz, self->n, fp) == self->n && result;
	return result-1;
}

static PyObject *
karray_store(PyKArray *self, PyObject *args) {
	PyFileObject* f = NULL;
	int result = TRUE;

	if (! PyArg_ParseTuple(args, "O!:store", &PyFile_Type, &f))
		return NULL;

	FILE *fp = PyFile_AsFile((PyObject*)f);
	PyFile_IncUseCount(f);
	Py_BEGIN_ALLOW_THREADS
	
	result = PyKArray_store(self, fp) == 0;
	
	Py_END_ALLOW_THREADS
	PyFile_DecUseCount(f);
	
	if (!result) {
		PyErr_SetString(KapowArrayError, "Error writing to file.");
		return NULL;
	}
	
	Py_RETURN_NONE;
}


PyDoc_STRVAR(load__doc__,
"A.load(f) --> A\n"
"\n"
"Loads the array from the given file f in the format used by store().\n"
"Returns a reference to the given array.\n");

int
PyKArray_load(PyKArray *self, FILE *fp) {
	int result = 1;
	unsigned long nn = 0;
	unsigned int sz = 0;
	char magic[4];

	Py_BEGIN_ALLOW_THREADS
	result = fread(magic, 4, 1, fp) == 1 && result;
	result = fread(&nn, sizeof(nn), 1, fp) == 1  && result;
	result = fread(&sz, sizeof(sz), 1, fp) == 1  && result;
	Py_END_ALLOW_THREADS

	if (result) {
		PyKArray_clear(self);
		PyKArray_resize(self, nn, sz);
		self->fldsz = sz;
		self->n = nn;
		Py_BEGIN_ALLOW_THREADS
		result = fread(self->buf, self->fldsz, self->n, fp) == self->n && result;
		Py_END_ALLOW_THREADS
	}
	return result - 1;
}

static PyObject *
karray_load(PyKArray *self, PyObject *args) {
	PyFileObject* f = NULL;
	int result = TRUE;

	if (! PyArg_ParseTuple(args, "O!:load", &PyFile_Type, &f))
		return NULL;

	FILE *fp = PyFile_AsFile((PyObject*)f);
	PyFile_IncUseCount(f);
	result = PyKArray_load(self, fp) == 0;
	PyFile_DecUseCount(f);
	
	if (!result) {
		PyErr_SetString(KapowArrayError, "Error loading from file.");
		return NULL;
	}
	
	Py_INCREF(self); // Devuelvo una nueva referencia al propio objeto
	return (PyObject *)self;
}


PyDoc_STRVAR(copy__doc__,
"A.copy(n=current_n, fldsz=current_fldsz, start=0) -> A2\n"
"\n"
"Builds a new Array A2 copying from current Array and returns it.\n");

static PyObject *
karray_copy(PyKArray *self, PyObject *args, PyObject *kwds) {
	unsigned long nn = self->n;
	unsigned int fldsz = self->fldsz;
	unsigned long st = 0;

	static char *kwlist[] = {"n", "fldsz", "start", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|kIk:copy", kwlist, &nn, &fldsz, &st))
		return NULL;

	// TODO: Raise error si fldsz == 0;

	// Check de la región dentro del array
	if (st * (unsigned long)(self->fldsz) + nn * (unsigned long)fldsz > self->n * (unsigned long)(self->fldsz)) {
		PyErr_Format(KapowArrayError, "The selected region to cpoy (start=%lu, len=%lu, fldsz=%u) runs beyond the current region (len=%lu, fldsz=%u).", st, nn, fldsz, self->n, self->fldsz);
		return NULL;
	}

	unsigned long nsz = nn*(unsigned long)fldsz;
	void* nbuf = NULL;
	if (nn) {
		nbuf = malloc(nsz);
		if (!nbuf) {
			PyErr_Format(KapowArrayError, "No enough memory to alloc %lu bytes in copy()", (unsigned long)nsz);
			return NULL;
		}
	}

	PyKArray *other = (PyKArray*)PyKArray_new();
	other->n = nn;
	other->fldsz = fldsz;
	other->buf = nbuf;
	if (nsz) {
		memcpy(nbuf, ((unsigned char*)self->buf) + st * self->fldsz, nsz);
	}

	return (PyObject *)other;
}


PyDoc_STRVAR(memcpy__doc__,
"A.memcpy(A2, dst=0, start=0, n=A2.size)\n"
"\n"
"Copies A2[start:n] to A[dst:...]. If the source array doesn't fit raises\n"
"an exception.\n");

static PyObject *
karray_memcpy(PyKArray *self, PyObject *args, PyObject *kwds) {
	unsigned long nn = -1UL, st = 0, dst = 0;
	PyKArray *other = NULL;

	static char *kwlist[] = {"arr", "dst", "start", "n", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!|kkk:memcpy", kwlist, &PyKArray_Type, &other, &dst, &st, &nn))
		return NULL;

	if (nn == -1UL) nn = other->n-st;

	// Check de la región dentro del array
	if (st > other->n || st+nn > other->n || dst > self->n) {
		PyErr_Format(KapowArrayError, "The selected region to copy (start=%lu, len=%lu, fldsz=%u) runs beyond the array limits.", st, nn, other->fldsz);
		return NULL;
	}
	st *= other->fldsz;
	nn *= other->fldsz;
	dst *= self->fldsz;

	if (dst + nn > PyKArray_GET_BUFSZ(self)) {
		PyErr_Format(KapowArrayError, "The selected region to copy (start=%lu, len=%lu, fldsz=%u) runs beyond the array limits.", st, nn, other->fldsz);
		return NULL;
	}

	if (nn) {
		memcpy((char*)(self->buf)+dst, (char*)(other->buf)+st, nn);
	}

	Py_RETURN_NONE;
}


PyDoc_STRVAR(max__doc__,
"A.max() --> int\n"
"\n"
"Returns the maximum value in the array or None if empty.\n");

unsigned PY_LONG_LONG PyKArray_max(PyKArray *self) {
	unsigned long i;
	unsigned PY_LONG_LONG mx = 0UL;

	if (self->fldsz == 1) {
		unsigned char* p = PyKArray_GET_BUF(self);
		for (i = self->n; i; i--,p++) if (*p > mx) mx = *p;
	} else if (self->fldsz == 2) {
		unsigned short* p = PyKArray_GET_BUF(self);
		for (i = self->n; i; i--,p++) if (*p > mx) mx = *p;
	} else if (self->fldsz == 4) {
		unsigned int* p = PyKArray_GET_BUF(self);
		for (i = self->n; i; i--,p++) if (*p > mx) mx = *p;
	} else if (self->fldsz == 8) {
		unsigned long long* p = PyKArray_GET_BUF(self);
		for (i = self->n; i; i--,p++) if (*p > mx) mx = *p;
	}
	return mx;
}

static PyObject *
karray_max(PyKArray *self) {
	if (self->n == 0)
		Py_RETURN_NONE;

	return Py_BuildValue("K", PyKArray_max(self));
}


PyDoc_STRVAR(min__doc__,
"A.min() --> int\n"
"\n"
"Returns the minimum value in the array or None if empty.\n");

PyAPI_FUNC(unsigned PY_LONG_LONG) PyKArray_min(PyKArray *self) {
	unsigned long i;
	unsigned PY_LONG_LONG mn = -1UL;

	if (self->fldsz == 1) {
		unsigned char* p = PyKArray_GET_BUF(self);
		for (i = self->n; i; i--,p++) if (*p < mn) mn = *p;
	} else if (self->fldsz == 2) {
		unsigned short* p = PyKArray_GET_BUF(self);
		for (i = self->n; i; i--,p++) if (*p < mn) mn = *p;
	} else if (self->fldsz == 4) {
		unsigned int* p = PyKArray_GET_BUF(self);
		for (i = self->n; i; i--,p++) if (*p < mn) mn = *p;
	} else if (self->fldsz == 8) {
		unsigned long long* p = PyKArray_GET_BUF(self);
		for (i = self->n; i; i--,p++) if (*p < mn) mn = *p;
	}
	return mn;
}

static PyObject *
karray_min(PyKArray *self) {
	if (self->n == 0)
		Py_RETURN_NONE;

	return Py_BuildValue("K", PyKArray_min(self));
}


static Py_ssize_t
karray_length(PyKArray *a) {
	return a->n;
}


/*********************** Array as buffer *************************/


static Py_ssize_t
karray_buffer_getrwbuf(PyKArray *self, Py_ssize_t index, const void **ptr) {
	if (index != 0) {
		PyErr_SetString(PyExc_SystemError, "accessing non-existent Array segment");
		return -1;
	}
	*ptr = PyKArray_GET_BUF(self);
	return PyKArray_GET_BUFSZ(self);
}

static Py_ssize_t
karray_buffer_getsegcount(PyKArray *self, Py_ssize_t *lenp) {
	if (lenp) *lenp = PyKArray_GET_BUFSZ(self);
	return 1;
}

static int
karray_buffer_getbuffer(PyKArray *self, Py_buffer *view, int flags) {
	int ret;
	if (!self->n) return -1;
	ret = PyBuffer_FillInfo(view, (PyObject*)self, self->buf, PyKArray_GET_BUFSZ(self), 0, flags);
	if (ret >= 0) {
		self->binded_from++;
	}
	return ret;
}

static void
karray_buffer_releasebuf(PyKArray *self, Py_buffer *view) {
	self->binded_from--;
}

/* Buffer methods */
static PyBufferProcs karray_as_buffer = {
	(readbufferproc)karray_buffer_getrwbuf,    /* bf_getreadbuffer */
	(writebufferproc)karray_buffer_getrwbuf,   /* bf_getwritebuffer */
	(segcountproc)karray_buffer_getsegcount,     /* bf_getsegcount */
	(charbufferproc)karray_buffer_getrwbuf,    /* bf_getcharbuffer */
	(getbufferproc)karray_buffer_getbuffer,      /* bf_getbuffer */
	(releasebufferproc)karray_buffer_releasebuf,        /* bf_releasebuffer */
};

/*********************** Array Iterator **************************/

typedef struct {
	PyObject_HEAD
	unsigned long it_index;
	PyKArray *it_karr; /* Set to NULL when iterator is exhausted */
} karrayiterobject;

static PyObject *karray_iter(PyObject *);
static void karrayiter_dealloc(karrayiterobject *);
static int karrayiter_traverse(karrayiterobject *, visitproc, void *);
static PyObject *karrayiter_next(karrayiterobject *);
static PyObject *karrayiter_len(karrayiterobject *);

PyDoc_STRVAR(length_hint_doc, "Private method returning the length len(Array(it)).");

static PyMethodDef karrayiter_methods[] = {
	{"__length_hint__", (PyCFunction)karrayiter_len, METH_NOARGS, length_hint_doc},
	{NULL,              NULL}           /* sentinel */
};

PyTypeObject PyKArrayIter_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"karrayiterator",                             /* tp_name */
	sizeof(karrayiterobject),                     /* tp_basicsize */
	0,                                          /* tp_itemsize */
	/* methods */
	(destructor)karrayiter_dealloc,               /* tp_dealloc */
	0,                                          /* tp_print */
	0,                                          /* tp_getattr */
	0,                                          /* tp_setattr */
	0,                                          /* tp_compare */
	0,                                          /* tp_repr */
	0,                                          /* tp_as_number */
	0,                                          /* tp_as_sequence */
	0,                                          /* tp_as_mapping */
	0,                                          /* tp_hash */
	0,                                          /* tp_call */
	0,                                          /* tp_str */
	PyObject_GenericGetAttr,                    /* tp_getattro */
	0,                                          /* tp_setattro */
	0,                                          /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
	0,                                          /* tp_doc */
	(traverseproc)karrayiter_traverse,            /* tp_traverse */
	0,                                          /* tp_clear */
	0,                                          /* tp_richcompare */
	0,                                          /* tp_weaklistoffset */
	PyObject_SelfIter,                          /* tp_iter */
	(iternextfunc)karrayiter_next,                /* tp_iternext */
	karrayiter_methods,                           /* tp_methods */
	0,                                          /* tp_members */
};


static PyObject *
karray_iter(PyObject *karr) {
	karrayiterobject *it;

	if (!PyKArray_Check(karr)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	it = PyObject_GC_New(karrayiterobject, &PyKArrayIter_Type);
	if (it == NULL)
		return NULL;
	it->it_index = 0;
	Py_INCREF(karr);
	it->it_karr = (PyKArray *)karr;
	_PyObject_GC_TRACK(it);
	return (PyObject *)it;
}

static void
karrayiter_dealloc(karrayiterobject *it) {
	_PyObject_GC_UNTRACK(it);
	Py_XDECREF(it->it_karr);
	PyObject_GC_Del(it);
}

static int
karrayiter_traverse(karrayiterobject *it, visitproc visit, void *arg) {
	Py_VISIT(it->it_karr);
	return 0;
}

static PyObject *
karrayiter_next(karrayiterobject *it) {
	PyKArray *karr;
	unsigned PY_LONG_LONG item;

	assert(it != NULL);
	karr = it->it_karr;
	if (karr == NULL)
		return NULL;
	assert(PyKArray_Check(karr));

	if (it->it_index < PyKArray_GET_SIZE(karr)) {
		item = PyKArray_GET_ITEM(karr, it->it_index);
		++it->it_index;
		return Py_BuildValue("K", item);
	}

	Py_CLEAR(it->it_karr);
	return NULL;
}

static PyObject *
karrayiter_len(karrayiterobject *it) {
	Py_ssize_t len;
	if (it->it_karr) {
		len = PyKArray_GET_SIZE(it->it_karr) - it->it_index;
		if (len >= 0)
			return PyInt_FromSsize_t(len);
	}
	return PyInt_FromLong(0);
}


/**** Mapping functions (operator[]) ****/

static PyObject *
karray_subscript(PyKArray* self, PyObject* item) {
	if (PyIndex_Check(item)) {
		Py_ssize_t i;
		i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		if (i < 0)
			i += PyKArray_GET_SIZE(self);
		if (i < 0 || i >= PyKArray_GET_SIZE(self)) {
			PyErr_Format(PyExc_IndexError,
				"index %lu out of range [0,%lu)", (unsigned long)i, (unsigned long)PyKArray_GET_SIZE(self));
			return NULL;
		}
		return Py_BuildValue("K", PyKArray_GET_ITEM(self, i));
	}
	else if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelen;
		
		if (PySlice_GetIndicesEx((PySliceObject*)item, PyKArray_GET_SIZE(self), &start, &stop, &step, &slicelen) < 0)
			return NULL;
		if (step != 1) {
			PyErr_SetString(PyExc_TypeError, "Array indices slices must have a step of 1.");
			return NULL;
		}
		return (PyObject*)PyKArray_bind(self, start, slicelen, self->fldsz);
	}
	else {
		PyErr_Format(PyExc_TypeError,
			"Array indices must be integers, not %.200s",
			item->ob_type->tp_name);
		return NULL;
	}
}

static int
karray_ass_subscript(PyKArray* self, PyObject* item, PyObject* value) {
	if (PyIndex_Check(item)) {
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return -1;
		if (i < 0)
			i += PyKArray_GET_SIZE(self);
		if (i < 0 || i >= PyKArray_GET_SIZE(self)) {
			PyErr_Format(PyExc_IndexError,
				"index %lu out of range [0,%lu)", (unsigned long)i, (unsigned long)PyKArray_GET_SIZE(self));
			return -1;
		}
		if (PyInt_Check(value)) {
			unsigned PY_LONG_LONG vl = PyInt_AsUnsignedLongLongMask(value);
			PyKArray_SET_ITEM(self, i, vl);
		} else if (PyLong_Check(value)) {
			unsigned PY_LONG_LONG vl = PyLong_AsUnsignedLongLong(value);
			PyKArray_SET_ITEM(self, i, vl);
		} else if (PyString_Check(value) && PyString_GET_SIZE(value) == 1) {
			unsigned PY_LONG_LONG vl = *((unsigned char*)PyString_AS_STRING(value));
			PyKArray_SET_ITEM(self, i, vl);
		} else {
			PyErr_Format(PyExc_TypeError,
				"Array values must be integers or chars, not %.200s",
				value->ob_type->tp_name);
			return -1;
		}
		return 0;
	}
	else {
		PyErr_Format(PyExc_TypeError,
			"Array indices must be integers, not %.200s",
			item->ob_type->tp_name);
		return -1;
	}
}

static PyMappingMethods karray_as_mapping = {
	(lenfunc)karray_length,
	(binaryfunc)karray_subscript,
	(objobjargproc)karray_ass_subscript
};


/*** Type-Definitions ***/

static PyMemberDef PyKArray_members[];
static PyMethodDef PyKArray_methods[];

PyTypeObject PyKArray_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"kapow.Array",             /*tp_name*/
	sizeof(PyKArray),        /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)PyKArray_dealloc,                         /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	(reprfunc)PyKArray_repr,   /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	&karray_as_mapping,        /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	(reprfunc)PyKArray_str,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	&karray_as_buffer,                        /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_NEWBUFFER,            /* tp_flags */
	"KAPOW Flat Array",           /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	karray_iter,                         /* tp_iter */
	0,                         /* tp_iternext */
	PyKArray_methods,             /* tp_methods */
	PyKArray_members,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)karray_init,     /* tp_init */
	0,                         /* tp_alloc */
	karray_new,                /* tp_new */
};

static PyMemberDef PyKArray_members[] = {
	{"size", T_ULONG, offsetof(PyKArray, n), READONLY, "Number of elements"},
	{"elemsize", T_UINT, offsetof(PyKArray, fldsz), READONLY, "Size of each element"},
	{"binded", T_OBJECT, offsetof(PyKArray, bind_to), READONLY, "The Array binded to, or None if not binded."},
	{NULL}  /* Sentinel */
};

static PyMethodDef PyKArray_methods[] = {
	_PyMethod(karray, memset, METH_VARARGS),
	_PyMethod(karray, fill, METH_VARARGS),
	_PyMethod(karray, resize, METH_VARARGS),
	_PyMethod(karray, clean, METH_NOARGS),
	_PyMethod(karray, inverse, METH_VARARGS),
	_PyMethod(karray, reverse, METH_VARARGS),
	_PyMethod(karray, bind, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(karray, unbind, METH_NOARGS),
	_PyMethod(karray, store, METH_VARARGS),
	_PyMethod(karray, load, METH_VARARGS),
	_PyMethod(karray, copy, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(karray, memcpy, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(karray, bindFrom, METH_VARARGS),
	_PyMethod(karray, max, METH_NOARGS),
	_PyMethod(karray, min, METH_NOARGS),
	{NULL}  /* Sentinel */
};

/* Type-Initialization */


void kapow_Array_init(PyObject* m) {

	PyKArray_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PyKArray_Type) < 0)
		return;

	Py_INCREF(&PyKArray_Type);
	PyModule_AddObject(m, "Array", (PyObject *)&PyKArray_Type);

	KapowArrayError = PyErr_NewException("kapow.KapowArrayError", NULL, NULL);
	Py_INCREF(KapowArrayError);
}

