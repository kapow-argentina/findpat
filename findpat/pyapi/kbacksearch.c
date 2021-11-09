#include <Python.h>
#include "structmember.h"
#include "kbacksearch.h"
#include "kmacros.h"
#include "tiempos.h"

#include "fastq.h"
#include "karray.h"

#define debug_call(METH, self) debug("oo PyKBackSearch: %9s %p\n", METH, (self))

/* Errores de Python */
static PyObject *KapowBackSearchError;

/* Tipo PyKBackSearch */

PyTypeObject PyKBackSearch_Type;

/* Constructors / Destructors */

PyObject *
PyKBackSearch_new(void) {
	PyKBackSearch *self;

	self = (PyKBackSearch *)PyObject_GC_New(PyKBackSearch, &PyKBackSearch_Type);
	if (self == NULL)
		return PyErr_NoMemory();
	
	//self->data = NULL;
	
	debug_call("NEW", self);
	
	return (PyObject *)self;
}

static PyObject *
kbacksearch_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyKBackSearch *self;

	/*
	if (! PyArg_ParseTuple(args, "...", &...))
		return NULL;
	*/

	self = (PyKBackSearch *)PyKBackSearch_new();

	return (PyObject *)self;
}

PyDoc_STRVAR(__init__doc__,
"KAPOW Backward Search\n"
"\n"
"R.__init__(src, r)\n"
"R.__init__(file)\n"
"\n"
"Implements a Backward Search algorithm for search patterns in src, given\n"
"the suffix array r for initializarion. If a file object is given, the\n"
"Backward Search structure is loaded from that file.\n");

int
PyKBackSearch_init(PyKBackSearch *self, uchar* src, uint* r, unsigned long n) {
	
	debug_call("INIT", self);
	
	backsearch_init((char*)src, r, n, &(self->data));
	
	return 0;
}

int
PyKBackSearch_init_load(PyKBackSearch *self, FILE* fp) {
	
	debug_call("INIT_LOAD", self);
	
	char magic[4];
	int result = fread(magic, 4, 1, fp) == 1;
	
	return (!backsearch_load(&self->data, fp) && result) -1;
}

static int
kbacksearch_init(PyKBackSearch *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc, *kr;
	PyFileObject *kfp;
	uchar* src;
	unsigned long n;
	uint* r;
	int res;
	FILE *fp;

	static char *kwlist[] = {"src", "r", NULL};
	static char *kwlist2[] = {"file", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO!:__init__", kwlist, &ksrc, &PyKArray_Type, &kr)) {
		PyErr_Clear();
		if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!:__init__", kwlist2, &PyFile_Type, &kfp))
			return -1;
		
		fp = PyFile_AsFile((PyObject*)kfp);
		PyFile_IncUseCount(kfp);
		Py_BEGIN_ALLOW_THREADS
		res = PyKBackSearch_init_load(self, fp);
		Py_END_ALLOW_THREADS
		PyFile_DecUseCount(kfp);

		if (!res) return 0;
		else {
			PyErr_SetString(KapowBackSearchError, "Error loading from file.");
			return -1;
		}
	}
	
	paramCheckKArrayStr_int(ksrc, src, n);
	paramCheckArraySize_int(kr, n, 4);
	
	r = PyKArray_GET_BUF(kr);
	
	return PyKBackSearch_init(self, src, r, n); // + args...
}


static void
kbacksearch_dealloc(register PyKBackSearch* self) {
	debug_call("DEALLOC", self);
	PyObject_GC_UnTrack(self);        // must untrack first

	backsearch_destroy(&(self->data));
/*
	Py_TRASHCAN_SAFE_BEGIN(self)
	... The body of the deallocator goes here, including all calls ...
	... to Py_DECREF on contained objects.                         ...
	Py_TRASHCAN_SAFE_END(self)
*/
	PyObject_GC_Del(self);
	// o esta otra?? Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
kbacksearch_traverse(PyKBackSearch *self, visitproc visit, void *arg) {
	// if (self->array) { Py_VISIT(self->array); }
	return 0;
}

/*** Methods ***/

PyDoc_STRVAR(store__doc__,
"BS.store(f)\n"
"\n"
"Stores the structure in the given file f (should be open for write). This\n"
"can be later loaded with the class constructor.\n");

static PyObject *
kbacksearch_store(PyKBackSearch *self, PyObject *args) {
	PyFileObject* f = NULL;
	int result = TRUE;

	if (! PyArg_ParseTuple(args, "O!:store", &PyFile_Type, &f))
		return NULL;

	FILE *fp = PyFile_AsFile((PyObject*)f);
	PyFile_IncUseCount(f);

	char magic[4] = "BS00";
	int uno = 1;
	magic[2] += sizeof(long);
	magic[3] += *(char*)&uno;

	Py_BEGIN_ALLOW_THREADS
	result = fwrite(magic, 4, 1, fp) == 1  && result;
	result = !backsearch_store(&self->data, fp) && result;
	Py_END_ALLOW_THREADS
	
	PyFile_DecUseCount(f);
	
	if (!result) {
		PyErr_SetString(KapowBackSearchError, "Error writing to file.");
		return NULL;
	}
	
	Py_RETURN_NONE;
}


PyDoc_STRVAR(search__doc__,
"BS.search(pat) -> from, to, time\n"
"\n"
"Searches for 'pat' in the original text.\n"
);

static PyObject *
kbacksearch_search(PyKBackSearch *self, PyObject *args, PyObject *kwds) {
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
	backsearch_locate(&(self->data), (char*)pat, pn, &vfrom, &vto);
	TIME_END;

	Py_END_ALLOW_THREADS

	return Py_BuildValue("IId", vfrom, vto, TIME_DIFF);
}

PyDoc_STRVAR(search_many__doc__,
"BS.search_many(pats, sep='\\000', from=None, to=None) -> total_count,time\n"
"\n"
"Searches each of the strings given in the string or kapow.Array 'pats'\n"
"delimiteds by a separator char 'sep', using the suffix array r of the\n"
"given src string or kapow.Array. src and r must have the same length.\n"
"If 'sep' is None, pats is assumed to be in FASTQ format.\n"
// "Returns the range [from,to) of r which contains the given pattern.\n"
);

static PyObject *
kbacksearch_search_many(PyKBackSearch *self, PyObject *args, PyObject *kwds) {
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
			backsearch_locate(&(self->data), (char*)fastq_it(it), fastq_len(it), &vfrom, &vto);
			cnt += (unsigned long)(vto-vfrom);
			//if (write(2, fastq_it(it), fastq_len(it))); fprintf(stderr, " #%d\n", vto-vfrom);
			*(vlfrom++) = vfrom;
			*(vlto++) = vto;
		}
	} else {
		for_line(it, pat, pn, sep) {
			backsearch_locate(&(self->data), (char*)line_it(it), line_len(it), &vfrom, &vto);
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

static PyMemberDef PyKBackSearch_members[];
static PyMethodDef PyKBackSearch_methods[];

PyTypeObject PyKBackSearch_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"kapow.BackSearch",             /*tp_name*/
	sizeof(PyKBackSearch),          /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)kbacksearch_dealloc,                         /*tp_dealloc*/
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
	__init__doc__,             /* tp_doc */
	(traverseproc)kbacksearch_traverse,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PyKBackSearch_methods,          /* tp_methods */
	PyKBackSearch_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)kbacksearch_init,     /* tp_init */
	0,                         /* tp_alloc */
	kbacksearch_new,                /* tp_new */
};

static PyMemberDef PyKBackSearch_members[] = {
	{"size", T_UINT, offsetof(PyKBackSearch, data.n), READONLY, "Text size"},
	{NULL}  /* Sentinel */
};

static PyMethodDef PyKBackSearch_methods[] = {
	_PyMethod(kbacksearch, search, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kbacksearch, search_many, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kbacksearch, store, METH_VARARGS),
	/*
	{"method", (PyCFunction)PyKBackSearch_method, METH_VARARGS, method__doc__},
	*/
	{NULL}  /* Sentinel */
};

/* Type-Initialization */

void kapow_BackSearch_init(PyObject* m) {

	PyKBackSearch_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PyKBackSearch_Type) < 0)
		return;

	Py_INCREF(&PyKBackSearch_Type);
	PyModule_AddObject(m, "BackSearch", (PyObject *)&PyKBackSearch_Type);

	KapowBackSearchError = PyErr_NewException("kapow.KapowBackSearchError", NULL, NULL);
	Py_INCREF(KapowBackSearchError);
}

