#include <Python.h>
#include "structmember.h"
#include "ksaindex.h"
#include "karray.h"
#include "kmacros.h"

#include "tiempos.h"
#include "macros.h"
#include "fastq.h"

#include "saindex.h"

#define debug_call(METH, self) debug("oo PyKSAIndex: %9s %p\n", METH, (self))


/* Errores de Python */
static PyObject *KapowSAIndexError;

/* Tipo PyKSAIndex */

PyTypeObject PyKSAIndex_Type;

/* Constructors / Destructors */

PyObject *
PyKSAIndex_new() {
	PyKSAIndex *self;

	self = (PyKSAIndex *)PyObject_GC_New(PyKSAIndex, &PyKSAIndex_Type);

	if (self == NULL)
		return PyErr_NoMemory();

	self->ks = NULL;
	memset(&self->sa, 0, sizeof(self->sa));

	debug_call("NEW", self);

	return (PyObject *)self;
}

static PyObject *
ksaindex_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyKSAIndex *self;
	self = (PyKSAIndex *)PyKSAIndex_new();
	return (PyObject *)self;
}

PyDoc_STRVAR(__init__doc__,
"KAPOW Suffix Array Index\n"
"\n"
"SA.__init__(src, lookup=0, flags=0)\n"
"SA.__init__(file)\n"
"\n"
"Builds a representation for the suffix array of the string src,\n"
"or loads it from a file.\n"
"The lookup parameter specifies the prefix length to store in the\n"
"lookup-table.\n"
"If the SA is build with flags=kapow.SA_STORE_P, the inverse lookup\n"
"inside the structure not allowing to call to p() or lookup_p().\n");

int
PyKSAIndex_init(PyKSAIndex *self, const unsigned char *s, unsigned int n, uint lookupsz, uint flags) {
	debug_call("INIT", self);

	int ret = 0;
	// TODO: Error checking

	saindex_init(&self->sa, s, n, lookupsz, flags);
	// if ((self->sa.flags & SA_NO_SRC) == 0 &&

	return ret;
}

int
PyKSAIndex_init_load(PyKSAIndex *self, FILE* fp) {
	debug_call("INIT_LOAD", self);

	int ret = 0;
	char magic[4];
	// TODO: Error checking

	ret -= fread(magic, 4, 1, fp) != 1; // TODO: Check signature
	ret += saindex_load(&self->sa, fp);
	self->ks = NULL;

	return ret;
}

static int
ksaindex_init(PyKSAIndex *self, PyObject *args, PyObject *kwds) {
	PyFileObject *kfp = NULL;
	int ret = 0;
	unsigned int flags = 0;
	unsigned int lookup = 0;
	FILE *fp;

	static char *kwlist[] = {"src", "lookup", "flags", NULL};
	static char *kwlist2[] = {"file", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!|II:__init__", kwlist, &PyKArray_Type, &self->ks, &lookup, &flags)) {
		PyErr_Clear();
		if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!:__init__", kwlist2, &PyFile_Type, &kfp))
			return -1;

		fp = PyFile_AsFile((PyObject*)kfp);
		PyFile_IncUseCount(kfp);
		Py_BEGIN_ALLOW_THREADS
		ret = PyKSAIndex_init_load(self, fp);
		Py_END_ALLOW_THREADS
		PyFile_DecUseCount(kfp);

		if (!ret) return 0;
		else {
			PyErr_SetString(KapowSAIndexError, "Error loading from file.");
			return -1;
		}
	}

	if (PyKArray_GET_ITEMSZ(self->ks) != 1) {
		PyErr_SetString(KapowSAIndexError, "The string must be a kapow.Array of elements of size 1.");
		return -1;
	}

	ret = PyKSAIndex_init(self, (unsigned char *)PyKArray_GET_BUF(self->ks), PyKArray_GET_SIZE(self->ks), lookup, flags);

	if (ret) {
		self->ks = NULL;
		PyErr_SetString(KapowSAIndexError, "Error creating the SAIndex (enough memory?).");
		return ret;
	}

	if ((flags & SA_STORE_S) || (flags & SA_DNA_ALPHA) || (flags & SA_NO_SRC)) {
		self->ks = NULL;
	} else {
		Py_INCREF(self->ks);
	}

	return 0;
}

static void
ksaindex_dealloc(register PyKSAIndex* self) {
	debug_call("DEALLOC", self);
	PyObject_GC_UnTrack(self);        // must untrack first

	saindex_destroy(&self->sa);

	Py_TRASHCAN_SAFE_BEGIN(self)
	Py_CLEAR(self->ks);
	Py_TRASHCAN_SAFE_END(self)
	PyObject_GC_Del(self);
}

static int
ksaindex_traverse(PyKSAIndex *self, visitproc visit, void *arg) {
	if (self->ks) { Py_VISIT(self->ks); }
	return 0;
}


PyDoc_STRVAR(store__doc__,
"SA.store(file)\n"
"\n"
"Stores the Suffix Array in the given file f (should be open for write).\n"
"This can be later loaded with the constructor.\n");

static PyObject *
ksaindex_store(PyKSAIndex *self, PyObject *args) {
	PyFileObject* f = NULL;
	int result = 1;

	if (! PyArg_ParseTuple(args, "O!:store", &PyFile_Type, &f))
		return NULL;

	FILE *fp = PyFile_AsFile((PyObject*)f);
	PyFile_IncUseCount(f);

	char magic[4] = "SA00";
	int uno = 1;
	magic[2] += sizeof(long);
	magic[3] += *(char*)&uno;

	Py_BEGIN_ALLOW_THREADS
	result = fwrite(magic, 4, 1, fp) == 1  && result;
	result = !saindex_store(&self->sa, fp) && result;
	Py_END_ALLOW_THREADS

	PyFile_DecUseCount(f);

	if (!result) {
		PyErr_SetString(KapowSAIndexError, "Error writing to file.");
		return NULL;
	}

	Py_RETURN_NONE;
}


PyDoc_STRVAR(get_r__doc__,
"SA.get_r() -> R\n"
"\n"
"Returns a referenced (bound) R array.\n"
);

static PyObject *
ksaindex_get_r(PyKSAIndex *self) {
	PyKArray* kr;
	kr = (PyKArray*)PyKArray_new();
	if (!kr) return NULL;
	Py_INCREF(self); // To bind it
	if (PyKArray_bindto(kr, (PyObject*)self, self->sa.r,  self->sa.n, 4) != 0) {
		Py_CLEAR(kr);
		return NULL;
	}
	return (PyObject*)kr;
}


PyDoc_STRVAR(r__doc__,
"SA.r(i) -> j\n"
"\n"
"Returns the i-th entry of the suffix array.\n"
"s[j:] is the i-th suffix in lexicographic order.\n"
);

static PyObject *
ksaindex_r(PyKSAIndex *self, PyObject *args, PyObject *kwds) {
	unsigned int i, n;
	if (!PyArg_ParseTuple(args, "I:comprsa", &i))
		return NULL;
	n = self->sa.n;
	if (i >= n) {
		PyErr_Format(PyExc_IndexError, "index %u out of range [0,%u)", i, n);
		return NULL;
	}
	return Py_BuildValue("I", self->sa.r[i]);
}

PyDoc_STRVAR(p__doc__,
"SA.p(j) -> i\n"
"\n"
"Returns the index of s[j:] in the suffix array.\n"
"There are i suffixes lesser than s[j:] in lexicographic order.\n"
);

static PyObject *
ksaindex_p(PyKSAIndex *self, PyObject *args, PyObject *kwds) {
	unsigned int i, n;
	if (!PyArg_ParseTuple(args, "I:p", &i))
		return NULL;
	if (self->sa.p == NULL) {
		PyErr_SetString(PyExc_IndexError, "Can't call p() in an object created without SA_STORE_P.");
		return NULL;
	}
	n = self->sa.n;
	if (i >= n) {
		PyErr_Format(PyExc_IndexError, "index %u out of range [0,%u)", i, n);
		return NULL;
	}
	return Py_BuildValue("I", self->sa.p[i]);
}

PyDoc_STRVAR(lookup_r__doc__,
"SA.lookup_r(from, to) -> (time, occs)\n"
"\n"
"Returns a kapow.Array containing SA.r(i) for each i in [from,to)\n"
);
static PyObject *
ksaindex_lookup_r(PyKSAIndex *self, PyObject *args, PyObject *kwds) {
	TIME_VARS;

	PyObject *res;
	unsigned int from, to, n;
	unsigned int *bf;

	if (!PyArg_ParseTuple(args, "II:lookup_r", &from, &to))
		return NULL;

	n = self->sa.n;
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
	memcpy(bf, self->sa.r+from, sizeof(uint) * (to-from));
	TIME_END;

	return Py_BuildValue("dN", TIME_DIFF, res);
}

PyDoc_STRVAR(lookup_many_r__doc__,
"SA.lookup_many_r(arrfrom, arrto) -> (time, occs)\n"
"\n"
"Returns a kapow.Array containing SA.r(i) for each i in [from,to), for\n"
"each pair from,to in arrfrom,arrto arrays\n"
);
static PyObject *
ksaindex_lookup_many_r(PyKSAIndex *self, PyObject *args, PyObject *kwds) {
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

	n = self->sa.n;
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
			*bf++ = self->sa.r[i];
		}
	}
	TIME_END;

	return Py_BuildValue("dN", TIME_DIFF, res);
}

PyDoc_STRVAR(lookup_p__doc__,
"SA.lookup_p(from, to) -> (time, occs)\n"
"\n"
"Returns a kapow.Array containing SA.p(i) for each i in [from,to)\n"
);
static PyObject *
ksaindex_lookup_p(PyKSAIndex *self, PyObject *args, PyObject *kwds) {
	TIME_VARS;

	PyObject *res;
	unsigned int from, to, i, n;
	unsigned int *bf;

	if (!PyArg_ParseTuple(args, "II:comprsa", &from, &to))
		return NULL;

	if (self->sa.p == NULL) {
		PyErr_SetString(PyExc_IndexError, "Can't lookup_p in an object created without SA_STORE_P.");
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

	TIME_START;
	for (i = from; i < to; i++) {
		*bf++ = self->sa.p[i];
	}
	TIME_END;

	return Py_BuildValue("dN", TIME_DIFF, res);
}


PyDoc_STRVAR(mmsearch__doc__,
"SA.search(pat) -> from, to, time\n"
"\n"
"Searches for 'pat' in the original text.\n"
);

static PyObject *
ksaindex_mmsearch(PyKSAIndex *self, PyObject *args, PyObject *kwds) {
	PyKArray *kpat = NULL;
	TIME_VARS;

	uint pn, vfrom, vto;
	uchar *pat;

	static char *kwlist[] = {"pat", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O:search", kwlist, &kpat))
		return NULL;

	paramCheckKArrayStr(kpat, pat, pn);

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	saindex_mmsearch(&self->sa, pat, pn, &vfrom, &vto);
	TIME_END;

	Py_END_ALLOW_THREADS

	return Py_BuildValue("IId", vfrom, vto, TIME_DIFF);
}

PyDoc_STRVAR(mmsearch_many__doc__,
"SA.search_many(pats, sep='\\000') -> total_count,time\n"
"\n"
"Searches each of the strings given in the string or kapow.Array 'pats'\n"
"delimiteds by a separator char 'sep', using the suffix array r of the\n"
"given src string or kapow.Array. src and r must have the same length.\n"
"If 'sep' is None, pats is assumed to be in FASTQ format.\n"
// "Returns the range [from,to) of r which contains the given pattern.\n"
);

static PyObject *
ksaindex_mmsearch_many(PyKSAIndex *self, PyObject *args, PyObject *kwds) {
	PyKArray *kpat = NULL, *afrom = NULL, *ato = NULL;
	PyObject* onone = NULL;
	int fastq = 0;
	TIME_VARS;

	uint pn, vfrom, vto;
	uchar *pat;
	uchar sep = 0;

	static char *kwlist[] = {"pats", "sep", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|c:search_many", kwlist, &kpat, &sep)) {
		PyErr_Clear();
		if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|O:search_many", kwlist, &kpat, &onone))
			return NULL;
		if (onone != Py_None) {
			PyErr_SetString(PyExc_TypeError, "sep must be a char or None");
			return NULL;
		}
		fastq = 1;
	}

	paramCheckKArrayStr(kpat, pat, pn);
	afrom = (PyKArray*)PyKArray_new();
	ato = (PyKArray*)PyKArray_new();

	unsigned long cnt = 0;
	unsigned long outsize = 0;

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	if (fastq) {
		for_fastq(it, pat, pn) outsize++;
	} else {
		for_line(it, pat, pn, sep) outsize++;
	}
	Py_END_ALLOW_THREADS

	uint *vlfrom, *vlto;
	if (PyKArray_resize(afrom, outsize, sizeof(*vlfrom)) < 0)
		return NULL;
	if (PyKArray_resize(ato, outsize, sizeof(*vlto)) < 0)
		return NULL;
	vlfrom = PyKArray_GET_BUF(afrom);
	vlto = PyKArray_GET_BUF(ato);

	Py_BEGIN_ALLOW_THREADS
	if (fastq) {
		for_fastq(it, pat, pn) {
			saindex_mmsearch(&self->sa, fastq_it(it), fastq_len(it), &vfrom, &vto);
			cnt += (unsigned long)(vto-vfrom);
			//if (write(2, fastq_it(it), fastq_len(it))); fprintf(stderr, " #%d\n", vto-vfrom);
			*(vlfrom++) = vfrom;
			*(vlto++) = vto;
		}
	} else {
		for_line(it, pat, pn, sep) {
			saindex_mmsearch(&self->sa, line_it(it), line_len(it), &vfrom, &vto);
			cnt += (unsigned long)(vto-vfrom);
			*(vlfrom++) = vfrom;
			*(vlto++) = vto;
		}
	}
	TIME_END;

	Py_END_ALLOW_THREADS

	return Py_BuildValue("NNkd", afrom, ato, cnt, TIME_DIFF);
}


/*** Type-Definitions ***/

static PyMemberDef PyKSAIndex_members[];
static PyMethodDef PyKSAIndex_methods[];

PyTypeObject PyKSAIndex_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"kapow.SAIndex",           /*tp_name*/
	sizeof(PyKSAIndex),        /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)ksaindex_dealloc,                         /*tp_dealloc*/
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
	(traverseproc)ksaindex_traverse,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PyKSAIndex_methods,        /* tp_methods */
	PyKSAIndex_members,        /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)ksaindex_init,   /* tp_init */
	0,                         /* tp_alloc */
	ksaindex_new,              /* tp_new */
};

static PyMemberDef PyKSAIndex_members[] = {
	{"s", T_OBJECT, offsetof(PyKSAIndex, ks), READONLY, "Underlying string."},
	{NULL}  /* Sentinel */
};

static PyMethodDef PyKSAIndex_methods[] = {
	_PyMethod(ksaindex, get_r, METH_NOARGS),
	_PyMethod(ksaindex, r, METH_VARARGS),
	_PyMethod(ksaindex, p, METH_VARARGS),
	_PyMethod(ksaindex, lookup_r, METH_VARARGS),
	_PyMethod(ksaindex, lookup_p, METH_VARARGS),
	_PyMethod(ksaindex, lookup_many_r, METH_VARARGS),
	_PyMethod(ksaindex, store, METH_VARARGS),
	_PyMethod(ksaindex, mmsearch, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(ksaindex, mmsearch_many, METH_VARARGS | METH_KEYWORDS),
	/*
	{"method", (PyCFunction)PyKSAIndex_method, METH_VARARGS, method__doc__},
	_PyMethod(ksaindex, otromethod, METH_VARARGS),
	*/
	{NULL}  /* Sentinel */
};

/* Type-Initialization */

void kapow_SAIndex_init(PyObject* m) {

	PyKSAIndex_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PyKSAIndex_Type) < 0)
		return;

	Py_INCREF(&PyKSAIndex_Type);
	PyModule_AddObject(m, "SAIndex", (PyObject *)&PyKSAIndex_Type);
	PyModule_AddIntMacro(m, SA_STORE_P);
	PyModule_AddIntMacro(m, SA_NO_LRLCP);
	PyModule_AddIntMacro(m, SA_DNA_ALPHA);
	PyModule_AddIntMacro(m, SA_STORE_S);
	PyModule_AddIntMacro(m, SA_STORE_LCP);
	PyModule_AddIntMacro(m, SA_NO_SRC);

	KapowSAIndexError = PyErr_NewException("kapow.KapowSAIndexError", NULL, NULL);
	Py_INCREF(KapowSAIndexError);
}
