#include <Python.h>
#include "structmember.h"
#include "kcomprsa.h"
#include "karray.h"
#include "kmacros.h"
#include "kcallback.h"

#include "tiempos.h"
#include "output_callbacks.h"
#include "macros.h"

#define debug_call(METH, self) debug("oo PyKComprsa: %9s %p\n", METH, (self))

/* init flags: */
#define CSA_NO_P 1

/* Errores de Python */
static PyObject *KapowComprsaError;

/* Tipo PyKComprsa */

PyTypeObject PyKComprsa_Type;

/* Constructors / Destructors */

PyObject *
PyKComprsa_new() {
	PyKComprsa *self;

	self = (PyKComprsa *)PyObject_GC_New(PyKComprsa, &PyKComprsa_Type);

	if (self == NULL)
		return PyErr_NoMemory();
	
	self->ks = NULL;
	self->comprsa = NULL;
	
	debug_call("NEW", self);
	
	return (PyObject *)self;
}

static PyObject *
kcomprsa_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyKComprsa *self;
	self = (PyKComprsa *)PyKComprsa_new();
	return (PyObject *)self;
}

PyDoc_STRVAR(__init__doc__,
"KAPOW Compressed Suffix Array\n"
"\n"
"C.__init__(src, r, flags=0)\n"
"C.__init__(file)\n"
"\n"
"Builds a compressed representation for the suffix array r of the string s,\n"
"or loads it from a file.\n"
"If the CSA is build with flags=kapow.CSA_NO_P, the string s is not stored\n"
"inside the structure not allowing to call to p() or lookup_p().\n");

int
PyKComprsa_init(PyKComprsa *self, unsigned char *s, unsigned int *r, unsigned int n) {
	self->comprsa = sa_compress(s, r, n);
	debug_call("INIT", self);
	return 0;
}

int
PyKComprsa_init_load(PyKComprsa *self, FILE* fp) {
	debug_call("INIT_LOAD", self);
	
	int ret = 0;
	char magic[4];
	
	ret -= fread(magic, 4, 1, fp) != 1;
	self->comprsa = sa_load(fp);
	ret -= self->comprsa == NULL;

	char l;
	ret -= fread(&l, 1, 1, fp) != 1;
	self->ks = NULL;
	if (l == 'S' && !ret) {
		self->ks = (PyKArray*)PyKArray_new();
		PyKArray_resize(self->ks, self->comprsa->n, 1);
		ret -= fread(PyKArray_GET_BUF(self->ks), 1, PyKArray_GET_BUFSZ(self->ks), fp) != PyKArray_GET_BUFSZ(self->ks);
	}
	
	return ret;
}

static int
kcomprsa_init(PyKComprsa *self, PyObject *args, PyObject *kwds) {
	PyKArray *kr = NULL;
	unsigned int *r = NULL;
	unsigned int n = 0;
	PyFileObject *kfp = NULL;
	int ret = 0;
	unsigned int flags = 0;
	FILE *fp;

	static char *kwlist[] = {"src", "r", "flags", NULL};
	static char *kwlist2[] = {"file", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!O!|I:__init__", kwlist, &PyKArray_Type, &self->ks, &PyKArray_Type, &kr, &flags)) {
		PyErr_Clear();
		if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!:__init__", kwlist2, &PyFile_Type, &kfp))
			return -1;
		
		fp = PyFile_AsFile((PyObject*)kfp);
		PyFile_IncUseCount(kfp);
		Py_BEGIN_ALLOW_THREADS
		ret = PyKComprsa_init_load(self, fp);
		Py_END_ALLOW_THREADS
		PyFile_DecUseCount(kfp);

		if (!ret) return 0;
		else {
			PyErr_SetString(KapowComprsaError, "Error loading from file.");
			return -1;
		}
	}

	if (PyKArray_GET_ITEMSZ(self->ks) != 1) {
		PyErr_SetString(KapowComprsaError, "The string must be a kapow.Array of elements of size 1.");
		return -1;
	}
	if (PyKArray_GET_ITEMSZ(kr) != 4) {
		PyErr_SetString(KapowComprsaError, "The suffix array must be a kapow.Array of elements of size 4.");
		return -1;
	}

	n = PyKArray_GET_SIZE(self->ks);
	if (PyKArray_GET_SIZE(kr) != n) {
		PyErr_SetString(KapowComprsaError, "The string and its suffix array should have the same size.");
		return -1;
	}

	/* XXX: creation of compressed suffix array frees the underlying array */
	if (kr->bind_to != NULL || kr->binded_from != 0) {
		PyErr_SetString(KapowComprsaError, "The suffix array cannot be bound to be able to compress it.");
		return -1;
	}
	r = (unsigned int *)PyKArray_GET_BUF(kr);
	kr->buf = 0;
	kr->n = 0;

	PyKComprsa_init(self, (unsigned char *)PyKArray_GET_BUF(self->ks), r, n);

	if (flags & CSA_NO_P) {
		self->ks = NULL;
	} else {
		Py_INCREF(self->ks);
	}
	
	return 0;
}

static void
kcomprsa_dealloc(register PyKComprsa* self) {
	debug_call("DEALLOC", self);
	PyObject_GC_UnTrack(self);        // must untrack first
	
	if (self->comprsa) {
		sa_destroy(self->comprsa);
		self->comprsa = NULL;
	}

	Py_TRASHCAN_SAFE_BEGIN(self)
	if (self->ks) {
		Py_CLEAR(self->ks);
	}
	Py_TRASHCAN_SAFE_END(self)
	PyObject_GC_Del(self);
}

static int
kcomprsa_traverse(PyKComprsa *self, visitproc visit, void *arg) {
	if (self->ks) { Py_VISIT(self->ks); }
	return 0;
}


PyDoc_STRVAR(store__doc__,
"C.store(file)\n"
"\n"
"Stores the Suffix Array in the given file f (should be open for write).\n"
"This can be later loaded with the constructor.\n");

static PyObject *
kcomprsa_store(PyKComprsa *self, PyObject *args) {
	PyFileObject* f = NULL;
	int result = 1;

	if (! PyArg_ParseTuple(args, "O!:store", &PyFile_Type, &f))
		return NULL;

	FILE *fp = PyFile_AsFile((PyObject*)f);
	PyFile_IncUseCount(f);

	char magic[4] = "CS00";
	int uno = 1;
	magic[2] += sizeof(long);
	magic[3] += *(char*)&uno;

	Py_BEGIN_ALLOW_THREADS
	result = fwrite(magic, 4, 1, fp) == 1  && result;
	result = !sa_store(self->comprsa, fp) && result;
	if (self->ks) {
		result = fwrite("S", 1, 1, fp) == 1  && result;
		result = fwrite(PyKArray_GET_BUF(self->ks), 1, PyKArray_GET_BUFSZ(self->ks), fp) == PyKArray_GET_BUFSZ(self->ks)  && result;
	} else {
		result = fwrite("_", 1, 1, fp) == 1  && result;
	}
	Py_END_ALLOW_THREADS
	
	PyFile_DecUseCount(f);
	
	if (!result) {
		PyErr_SetString(KapowComprsaError, "Error writing to file.");
		return NULL;
	}
	
	Py_RETURN_NONE;
}


PyDoc_STRVAR(r__doc__,
"C.r(i) -> j\n"
"\n"
"Returns the i-th entry of the suffix array.\n"
"s[j:] is the i-th suffix in lexicographic order.\n"
);

static PyObject *
PyKComprsa_r(PyKComprsa *self, PyObject *args, PyObject *kwds) {
	unsigned int i, n;
	if (!PyArg_ParseTuple(args, "I:comprsa", &i))
		return NULL;
	n = self->comprsa->n;
	if (i >= n) {
		PyErr_Format(PyExc_IndexError, "index %u out of range [0,%u)", i, n);
		return NULL;
	}
	return Py_BuildValue("I", sa_lookup(self->comprsa, i));
}

PyDoc_STRVAR(p__doc__,
"C.p(j) -> i\n"
"\n"
"Returns the index of s[j:] in the suffix array.\n"
"There are i suffixes lesser than s[j:] in lexicographic order.\n"
);

static PyObject *
PyKComprsa_p(PyKComprsa *self, PyObject *args, PyObject *kwds) {
	unsigned int i, n;
	if (!PyArg_ParseTuple(args, "I:p", &i))
		return NULL;
	if (self->ks == NULL) {
		PyErr_SetString(PyExc_IndexError, "Can't call p() in an object created with CSA_NO_P.");
		return NULL;
	}
	n = PyKArray_GET_SIZE(self->ks);
	if (i >= n) {
		PyErr_Format(PyExc_IndexError, "index %u out of range [0,%u)", i, n);
		return NULL;
	}
	return Py_BuildValue("I", sa_inv_lookup(PyKArray_GET_BUF(self->ks), self->comprsa, i));
}

PyDoc_STRVAR(lookup_r__doc__,
"C.lookup_r(from, to) -> (time, occs)\n"
"\n"
"Returns a kapow.Array containing C.r(i) for each i in [from,to)\n"
);
static PyObject *
PyKComprsa_lookup_r(PyKComprsa *self, PyObject *args, PyObject *kwds) {
	TIME_VARS;

	PyObject *res;
	unsigned int from, to, i, n;
	unsigned int *bf;

	if (!PyArg_ParseTuple(args, "II:lookup_r", &from, &to))
		return NULL;

	n = self->comprsa->n;
	if (from > n) {
		PyErr_Format(PyExc_IndexError, "'from' (%u) out of range [0,%u]", from, n);
		return NULL;
	}
	if (to > n) {
		PyErr_Format(PyExc_IndexError, "'to' (%u) out of range [0,%u]", to, n);
		return NULL;
	}
	if (from > to) {
		PyErr_Format(PyExc_IndexError, "'from' (%u) should be lesser than 'to' (%u)", from, to);
		return NULL;
	}

	res = PyKArray_new();
	PyKArray_init((PyKArray *)res, to - from, 4);
	bf = PyKArray_GET_BUF(res);

	TIME_START;
	for (i = from; i < to; i++) {
		*bf++ = sa_lookup(self->comprsa, i);
	}
	TIME_END;

	return Py_BuildValue("dN", TIME_DIFF, res);
}

PyDoc_STRVAR(lookup_many_r__doc__,
"C.lookup_many_r(arrfrom, arrto) -> (time, occs)\n"
"\n"
"Returns a kapow.Array containing C.r(i) for each i in [from,to), for\n"
"each pair from,to in arrfrom,arrto arrays\n"
);
static PyObject *
PyKComprsa_lookup_many_r(PyKComprsa *self, PyObject *args, PyObject *kwds) {
	TIME_VARS;

	PyObject *res;
	PyKArray *afrom, *ato;
	unsigned int *from, *to, i, j, n, m;
	unsigned int *bf;
	unsigned long occs = 0;

	if (!PyArg_ParseTuple(args, "O!O!:lookup_many_r", &PyKArray_Type, &afrom, &PyKArray_Type, &ato))
		return NULL;
	
	if (PyKArray_GET_SIZE(afrom) != PyKArray_GET_SIZE(ato)) {
		PyErr_SetString(PyExc_IndexError, "'from' and 'to' must have the same size");
		return NULL;
	}

	if (PyKArray_GET_ITEMSZ(afrom) != 4 || PyKArray_GET_ITEMSZ(ato) != 4) {
		PyErr_SetString(PyExc_IndexError, "'from' and 'to' must be an array of ints");
		return NULL;
	}

	n = self->comprsa->n;
	m = PyKArray_GET_SIZE(afrom);

	TIME_START;
	from = PyKArray_GET_BUF(afrom);
	to = PyKArray_GET_BUF(ato);
	for(j=0; j<m; j++,from++,to++) {
		if (*from > n) {
			PyErr_Format(PyExc_IndexError, "'from' (%u) out of range [0,%u]", *from, n);
			return NULL;
		}
		if (*to > n) {
			PyErr_Format(PyExc_IndexError, "'to' (%u) out of range [0,%u]", *to, n);
			return NULL;
		}
		if (*from > *to) {
			PyErr_Format(PyExc_IndexError, "'from' (%u) should be lesser than 'to' (%u)", *from, *to);
			return NULL;
		}
		occs += *to-*from;
	}

	res = PyKArray_new();
	PyKArray_init((PyKArray *)res, occs, 4);
	bf = PyKArray_GET_BUF(res);

	from = PyKArray_GET_BUF(afrom);
	to = PyKArray_GET_BUF(ato);
	for(j=0; j<m; j++,from++,to++) {
		for (i = *from; i < *to; i++) {
			*bf++ = sa_lookup(self->comprsa, i);
		}
	}
	TIME_END;

	return Py_BuildValue("dN", TIME_DIFF, res);
}

PyDoc_STRVAR(lookup_p__doc__,
"C.lookup_p(from, to) -> (time, occs)\n"
"\n"
"Returns a kapow.Array containing C.p(i) for each i in [from,to)\n"
);
static PyObject *
PyKComprsa_lookup_p(PyKComprsa *self, PyObject *args, PyObject *kwds) {
	TIME_VARS;

	PyObject *res;
	unsigned int from, to, i, n;
	unsigned int *bf;
	unsigned char *s;

	if (!PyArg_ParseTuple(args, "II:comprsa", &from, &to))
		return NULL;

	if (self->ks == NULL) {
		PyErr_SetString(PyExc_IndexError, "Can't lookup_p in an object created with CSA_NO_P.");
		return NULL;
	}

	n = PyKArray_GET_SIZE(self->ks);
	if (from > n) {
		PyErr_Format(PyExc_IndexError, "'from' (%u) out of range [0,%u]", from, n);
		return NULL;
	}
	if (to > n) {
		PyErr_Format(PyExc_IndexError, "'to' (%u) out of range [0,%u]", to, n);
		return NULL;
	}
	if (from > to) {
		PyErr_Format(PyExc_IndexError, "'from' (%u) should be lesser than 'to' (%u)", from, to);
		return NULL;
	}

	res = PyKArray_new();
	PyKArray_init((PyKArray *)res, to - from, 4);
	bf = PyKArray_GET_BUF(res);
	s = PyKArray_GET_BUF(self->ks);

	TIME_START;
	for (i = from; i < to; i++) {
		*bf++ = sa_inv_lookup(s, self->comprsa, i);
	}
	TIME_END;

	return Py_BuildValue("dN", TIME_DIFF, res);
}

/*** Type-Definitions ***/

static PyMemberDef PyKComprsa_members[];
static PyMethodDef PyKComprsa_methods[];

PyTypeObject PyKComprsa_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"kapow.Comprsa",           /*tp_name*/
	sizeof(PyKComprsa),        /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)kcomprsa_dealloc,                         /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,        /*tp_flags*/
#if 0
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
#endif
	__init__doc__,             /* tp_doc */
	(traverseproc)kcomprsa_traverse,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PyKComprsa_methods,        /* tp_methods */
	PyKComprsa_members,        /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)kcomprsa_init,   /* tp_init */
	0,                         /* tp_alloc */
	kcomprsa_new,              /* tp_new */
};

static PyMemberDef PyKComprsa_members[] = {
	{"s", T_OBJECT, offsetof(PyKComprsa, ks), READONLY, "Underlying string."},
	{NULL}  /* Sentinel */
};

static PyMethodDef PyKComprsa_methods[] = {
	_PyMethod(PyKComprsa, r, METH_VARARGS),
	_PyMethod(PyKComprsa, p, METH_VARARGS),
	_PyMethod(PyKComprsa, lookup_r, METH_VARARGS),
	_PyMethod(PyKComprsa, lookup_p, METH_VARARGS),
	_PyMethod(PyKComprsa, lookup_many_r, METH_VARARGS),
	_PyMethod(kcomprsa, store, METH_VARARGS),
	/*
	{"method", (PyCFunction)PyKComprsa_method, METH_VARARGS, method__doc__},
	_PyMethod(kcomprsa, otromethod, METH_VARARGS),
	*/
	{NULL}  /* Sentinel */
};

/* Type-Initialization */

void kapow_Comprsa_init(PyObject* m) {

	PyKComprsa_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PyKComprsa_Type) < 0)
		return;

	Py_INCREF(&PyKComprsa_Type);
	PyModule_AddObject(m, "Comprsa", (PyObject *)&PyKComprsa_Type);
	PyModule_AddIntMacro(m, CSA_NO_P);

	KapowComprsaError = PyErr_NewException("kapow.KapowComprsaError", NULL, NULL);
	Py_INCREF(KapowComprsaError);
}

