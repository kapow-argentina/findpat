#include <Python.h>
#include "structmember.h"
#include "kpwfile.h"
#include "kpwapi.h"
#include "kmacros.h"
#include "karray.h"

#include "common.h"
#include "macros.h"

#define debug_call(METH, self) debug("oo PyKpwfile: %9s %p, kpw=%p\n", METH, (self), (self)->kpw)

/* Errores de Python */
static PyObject *KapowKpwfileError;

/************************* KPWFile section ****************************/

/* Struct del tipo */

typedef struct str_PyKpwsection {
	PyObject_HEAD
	kpw_index_ent sec;
	kpw_idx_it it;
	PyKpwfile* kpw;
} PyKpwsection;

PyTypeObject PyKpwfilesec_Type;

/* Constructors / Destructors */

PyObject *
PyKpwsection_new(PyKpwfile *kpw, kpw_idx_it it) {
	PyKpwsection *self;
	
	if (!kpw)
		return NULL; // ...y curtite
	
	self = (PyKpwsection *)PyObject_GC_New(PyKpwsection, &PyKpwfilesec_Type);
	if (self == NULL)
		return PyErr_NoMemory();

	Py_INCREF(kpw);
	self->kpw = kpw;
	self->it = it;
	self->sec = *KPW_IT(it);
	
//	debug_call("NEW", self);
	
	return (PyObject *)self;
}

/*
static PyObject *
kpwsection_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyKpwsection *self;

	self = (PyKpwsection*)PyKpwsection_new();

	return (PyObject *)self;
}
*/

/* Methods */

PyDoc_STRVAR(sec_read__doc__,
"S.read()\n"
"\n"
"Returns a kapow.Array() with the contents of the section.\n");

static PyObject*
kpwfilesec_read(PyKpwsection *ksec) {
	PyKArray* karr = NULL;
	void * buf;

	if (ksec->sec.size == 0 || ksec->sec.type == KPW_ID_EMPTY) {
		Py_RETURN_NONE;
	}

	uint64 size;
	buf = kpw_idx_sec_load(ksec->kpw->kpw, &(ksec->it), &size);
	if (!buf) {
		PyErr_Format(KapowKpwfileError, "Error loading section from disk.");
		return NULL;
	}
	if (size != ksec->sec.size) {
		pz_free(buf);
		PyErr_Format(KapowKpwfileError, "Error loading section from disk: not all bytes readed (%lu of %lu).", (unsigned long)size, (unsigned long)(ksec->sec.size));
		return NULL;
	}

	karr = (PyKArray*)PyKArray_new();
	if (!karr) return NULL;
	karr->buf = buf;
	karr->fldsz = 1;
	karr->n = size;
	
	return (PyObject*)karr;
}

static int
kpwfilesec_clear(PyKpwsection* self) {
//	debug_call("CLEAR", self);
	Py_CLEAR(self->kpw);
	return 0;
}

static void
kpwfilesec_dealloc(PyKpwsection *ksec) {
//	debug_call("DEALLOC", ksec);
	PyObject_GC_UnTrack(ksec);
	Py_TRASHCAN_SAFE_BEGIN(ksec)
	Py_XDECREF(ksec->kpw);
	Py_TRASHCAN_SAFE_END(ksec)
	PyObject_GC_Del(ksec);
}

static int
kpwfilesec_traverse(PyKpwsection *ksec, visitproc visit, void *arg) {
	Py_VISIT(ksec->kpw);
	return 0;
}


static PyMethodDef kpwfilesec_methods[] = {
	{"read", (PyCFunction)kpwfilesec_read, METH_NOARGS, sec_read__doc__},
	{NULL,              NULL}           /* sentinel */
};

static PyMemberDef kpwfilesec_members[] = {
	{"size", T_ULONGLONG, offsetof(PyKpwsection, sec.size), READONLY, "Section size."},
	{"type", T_UINT, offsetof(PyKpwsection, sec.type), READONLY, "Section type."},
	{NULL}  /* Sentinel */
};

PyTypeObject PyKpwfilesec_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"kpwfilesection",                             /* tp_name */
	sizeof(PyKpwsection),                     /* tp_basicsize */
	0,                                          /* tp_itemsize */
	/* methods */
	(destructor)kpwfilesec_dealloc,               /* tp_dealloc */
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
	(traverseproc)kpwfilesec_traverse,            /* tp_traverse */
	(inquiry)kpwfilesec_clear,                  /* tp_clear */
	0,                                          /* tp_richcompare */
	0,                                          /* tp_weaklistoffset */
	0,                                          /* tp_iter */
	0,                                          /* tp_iternext */
	kpwfilesec_methods,                         /* tp_methods */
	kpwfilesec_members,                         /* tp_members */
};


/****************************** KPWFile *******************************/

/* Prototipos */

PyTypeObject PyKpwfile_Type;

/* Constructors / Destructors */

PyObject *
PyKpwfile_new(void) {
	PyKpwfile *self;
	
	self = (PyKpwfile *)(&PyKpwfile_Type)->tp_alloc(&PyKpwfile_Type, 0);
	if (self != NULL) {
		self->kpw = NULL;
	}

	debug_call("NEW", self);

	return (PyObject *)self;
}

static PyObject *
kpwfile_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyKpwfile *self;

	self = (PyKpwfile*)PyKpwfile_new();

	return (PyObject *)self;
}

int
PyKpwfile_init(PyKpwfile *self, const char* filename) {
	int res = 0;
	
	self->kpw = kpw_open(filename);
	if (!self->kpw) {
		PyErr_Format(KapowKpwfileError, "Error opening KPW file «%s»", filename);
		res = -1;
	}

	debug_call("INIT", self);
	return res;
}

static int
kpwfile_init(PyKpwfile *self, PyObject *args, PyObject *kwds) {
	PyStringObject* fn = NULL;
	
	if (!PyArg_ParseTuple(args, "S:KPWFile", &fn))
		return -1;

	if (PyKpwfile_init(self, PyString_AS_STRING(fn)) < 0)
		return -1;

	return 0;
}


static int
PyKpwfile_clear(PyKpwfile* self) {
	debug_call("CLEAR", self);
	if (self->kpw) {
		kpw_close(self->kpw);
		self->kpw = NULL;
	}
	return 0;
}

static void
PyKpwfile_dealloc(PyKpwfile* self) {
	debug_call("DEALLOC", self);
	self->ob_type->tp_free((PyObject*)self);
}

/*** Methods ***/

PyDoc_STRVAR(list__doc__,
"K.list()\n"
"\n"
"Lists the contents of the file. Used for debug purposes.\n");

static PyObject*
kpwfile_list(PyKpwfile *self) {
	kpw_list(self->kpw);
	Py_RETURN_NONE;
}


PyDoc_STRVAR(load_stats__doc__,
"K.load_stats() -> STATS\n"
"\n"
"Returns the STATS section, or None if not exists.\n");

static PyObject*
kpwfile_load_stats(PyKpwfile *self) {
	PyObject* res;
	ppz_stats data;
	memset(&data, 0, sizeof(data));
	if (!kpw_load_stats(self->kpw, &data, sizeof(data))) {
		Py_RETURN_NONE;
	}
	
	res = PyKArray_new();
	if (!res) return NULL;
	if (PyKArray_init((PyKArray*)res, sizeof(ppz_stats), 1) < 0)
		return NULL;
	memcpy(PyKArray_GET_BUF(res), &data, sizeof(data));
	return res;
}

PyDoc_STRVAR(save_stats__doc__,
"K.save_stats(STATS)\n"
"\n"
"Updates the given STATS kapow.Array() to the KPW file.\n");

static PyObject*
kpwfile_save_stats(PyKpwfile *self, PyObject* args) {
	PyKArray* data;
	
	if (!PyArg_ParseTuple(args, "O!:save_stats", &PyKArray_Type, &data))
		return NULL;
	
	if (PyKArray_GET_BUFSZ(data) != sizeof(ppz_stats)) {
		PyErr_Format(KapowKpwfileError, "The given STATS array must have the correct size (%lu bytes).", (unsigned long)sizeof(data));
		return NULL;
	}
	
	kpw_save_stats(self->kpw, PyKArray_GET_BUF(data));
	Py_RETURN_NONE;
}


PyDoc_STRVAR(load_trac__doc__,
"K.load_trac() -> TRAC, trac_middle\n"
"\n"
"Returns the TRAC section and trac_middle, or None if not exists.\n");

static PyObject*
kpwfile_load_trac(PyKpwfile *self) {
	PyObject* res, *arr;
	uint trac_middle, trac_size, *trac;

	if (!(trac = kpw_load_trac(self->kpw, &trac_size, &trac_middle))) {
		Py_RETURN_NONE;
	}
	
	arr = PyKArray_new();
	if (!arr) return NULL;
	PyKArray_GET_BUF(arr) = trac;
	PyKArray_GET_ITEMSZ(arr) = 4;
	PyKArray_GET_SIZE(arr) = trac_size*2;
	
	res = Py_BuildValue("NI", arr, trac_middle);
	if (!res) Py_DECREF(arr);
	return res;
}

PyDoc_STRVAR(save_trac__doc__,
"K.save_trac(TRAC, trac_middle)\n"
"\n"
"Saves a new TRAC section to the KPW file.\n");

static PyObject*
kpwfile_save_trac(PyKpwfile *self, PyObject* args) {
	PyKArray* arr;
	uint trac_middle;
	
	if (!PyArg_ParseTuple(args, "O!I:save_trac", &PyKArray_Type, &arr, &trac_middle))
		return NULL;
	
	paramCheckArrayFldsz(arr, 4);
	
	if (!kpw_save_trac(self->kpw, PyKArray_GET_BUF(arr), PyKArray_GET_SIZE(arr)*2, trac_middle))
		return NULL;
	Py_RETURN_NONE;
}


PyDoc_STRVAR(load_lcp__doc__,
"K.load_lcp() -> lcp\n"
"\n"
"Returns the LCP section, or None if not exists.\n");

static PyObject*
kpwfile_load_lcp(PyKpwfile *self) {
	PyObject* arr;
	uint *lcp;
	unsigned long long int lcp_len = 0;

	if (!(lcp = kpw_load_lcp(self->kpw, &lcp_len))) {
		Py_RETURN_NONE;
	}
	
	arr = PyKArray_new();
	if (!arr) return NULL;
	PyKArray_GET_BUF(arr) = lcp;
	PyKArray_GET_ITEMSZ(arr) = 4;
	PyKArray_GET_SIZE(arr) = lcp_len;
	return arr;
}

PyDoc_STRVAR(save_lcp__doc__,
"K.save_lcp(LCP)\n"
"\n"
"Saves a new LCP section to the KPW file.\n");

static PyObject*
kpwfile_save_lcp(PyKpwfile *self, PyObject* args) {
	PyKArray* arr;
	
	if (!PyArg_ParseTuple(args, "O!:save_lcp", &PyKArray_Type, &arr))
		return NULL;
	
	paramCheckArrayFldsz(arr, 4);
	
	if (!kpw_save_lcp(self->kpw, PyKArray_GET_BUF(arr), PyKArray_GET_SIZE(arr)))
		return NULL;
	Py_RETURN_NONE;
}


PyDoc_STRVAR(load_lcpset__doc__,
"K.load_lcpset() -> LCPS,n\n"
"\n"
"Returns the LCPS section, or None if not exists.\n");

static PyObject*
kpwfile_load_lcpset(PyKpwfile *self) {
	PyObject* arr, *res;
	uchar *lcpset;
	unsigned long long int lcpset_len = 0;

	if (!(lcpset = kpw_load_lcpset(self->kpw, &lcpset_len))) {
		Py_RETURN_NONE;
	}
	
	arr = PyKArray_new();
	if (!arr) return NULL;
	PyKArray_GET_BUF(arr) = lcpset;
	PyKArray_GET_ITEMSZ(arr) = 1;
	PyKArray_GET_SIZE(arr) = (lcpset_len-1+7)/8;

	res = Py_BuildValue("NK", arr, lcpset_len);
	if (!res) Py_DECREF(arr);
	return res;
}

PyDoc_STRVAR(save_lcpset__doc__,
"K.save_lcpset(LCPS, n)\n"
"\n"
"Saves a new LCPS section to the KPW file.\n");

static PyObject*
kpwfile_save_lcpset(PyKpwfile *self, PyObject* args) {
	PyKArray* arr;
	unsigned long long int lcpset_len = 0;
	
	if (!PyArg_ParseTuple(args, "O!K:save_lcpset", &PyKArray_Type, &arr, &lcpset_len))
		return NULL;
	
	paramCheckArrayFldsz(arr, 1);
	if (PyKArray_GET_SIZE(arr) != (lcpset_len-1+7)/8) {
		PyErr_SetString(KapowKpwfileError, "The LCPS array must have (n-1+7)/8 bytes.");
		return NULL;
	}
	
	if (!kpw_save_lcpset(self->kpw, PyKArray_GET_BUF(arr), lcpset_len))
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(load_bwt__doc__,
"K.load_bwt() -> BWT,prim\n"
"\n"
"Returns the BWT section, or None if not exists.\n");

static PyObject*
kpwfile_load_bwt(PyKpwfile *self) {
	PyObject* arr, *res;
	uchar *bwt;
	uint bwt_len = 0, bwt_prim = 0;

	if (!(bwt = kpw_load_bwt(self->kpw, &bwt_len, &bwt_prim))) {
		Py_RETURN_NONE;
	}
	
	arr = PyKArray_new();
	if (!arr) return NULL;
	PyKArray_GET_BUF(arr) = bwt;
	PyKArray_GET_ITEMSZ(arr) = 1;
	PyKArray_GET_SIZE(arr) = bwt_len;

	res = Py_BuildValue("NI", arr, bwt_prim);
	if (!res) Py_DECREF(arr);
	return res;
}

PyDoc_STRVAR(save_bwt__doc__,
"K.save_bwt(BWT, prim)\n"
"\n"
"Saves a new BWT section to the KPW file.\n");

static PyObject*
kpwfile_save_bwt(PyKpwfile *self, PyObject* args) {
	PyKArray* arr;
	uint bwt_prim = 0;
	
	if (!PyArg_ParseTuple(args, "O!I:save_bwt", &PyKArray_Type, &arr, &bwt_prim))
		return NULL;
	
	paramCheckArrayFldsz(arr, 1);
	
	if (!kpw_save_bwt(self->kpw, PyKArray_GET_BUF(arr), PyKArray_GET_SIZE(arr), bwt_prim))
		return NULL;
	Py_RETURN_NONE;
}

/*********************** KPWFile Iterator **************************/

typedef struct {
	PyObject_HEAD
	kpw_idx_it it_idx;
	PyKpwfile *it_kpw; /* Set to NULL when iterator is exhausted */
} kpwfileiterobject;

static PyObject *kpwfile_iter(PyObject *);
static void kpwfileiter_dealloc(kpwfileiterobject *);
static int kpwfileiter_traverse(kpwfileiterobject *, visitproc, void *);
static PyObject *kpwfileiter_next(kpwfileiterobject *);

//PyDoc_STRVAR(length_hint_doc, "Private method returning the length len(Array(it)).");

static PyMethodDef kpwfileiter_methods[] = {
//	{"__length_hint__", (PyCFunction)kpwfileiter_len, METH_NOARGS, length_hint_doc},
	{NULL,              NULL}           /* sentinel */
};

PyTypeObject PyKpwfileIter_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"kpwfileiterator",                             /* tp_name */
	sizeof(kpwfileiterobject),                     /* tp_basicsize */
	0,                                          /* tp_itemsize */
	/* methods */
	(destructor)kpwfileiter_dealloc,               /* tp_dealloc */
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
	(traverseproc)kpwfileiter_traverse,            /* tp_traverse */
	0,                                          /* tp_clear */
	0,                                          /* tp_richcompare */
	0,                                          /* tp_weaklistoffset */
	PyObject_SelfIter,                          /* tp_iter */
	(iternextfunc)kpwfileiter_next,                /* tp_iternext */
	kpwfileiter_methods,                           /* tp_methods */
	0,                                          /* tp_members */
};


static PyObject *
kpwfile_iter(PyObject *kpw) {
	kpwfileiterobject *it;

	if (!PyKpwfile_Check(kpw)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	it = PyObject_GC_New(kpwfileiterobject, &PyKpwfileIter_Type);
	if (it == NULL)
		return NULL;
	it->it_idx = kpw_idx_begin(((PyKpwfile *)kpw)->kpw);
	Py_INCREF(kpw);
	it->it_kpw = (PyKpwfile *)kpw;
	_PyObject_GC_TRACK(it);
	return (PyObject *)it;
}

static void
kpwfileiter_dealloc(kpwfileiterobject *it) {
	_PyObject_GC_UNTRACK(it);
	Py_XDECREF(it->it_kpw);
	PyObject_GC_Del(it);
}

static int
kpwfileiter_traverse(kpwfileiterobject *it, visitproc visit, void *arg) {
	Py_VISIT(it->it_kpw);
	return 0;
}

static PyObject *
kpwfileiter_next(kpwfileiterobject *it) {
	PyKpwfile *kpw;
	PyObject *res = NULL;

	assert(it != NULL);
	kpw = it->it_kpw;
	if (kpw == NULL)
		return NULL;
	assert(PyKpwfile_Check(kpw));

	/* Finish? */
	if (KPW_IT_END(it->it_idx)) {
		Py_DECREF(kpw);
		it->it_kpw = NULL;
		return NULL;
	}

	res = PyKpwsection_new(kpw, it->it_idx);

	//kpw_index_ent* item = KPW_IT(it->it_idx);
	if (res) kpw_idx_next(&(it->it_idx));

	return res;
}

/**** Mapping functions (operator[]) ****/

static PyObject *
kpwfile_subscript(PyKpwfile* self, PyObject* item) {
	if (PyIndex_Check(item)) {
		Py_ssize_t i;
		i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		kpw_idx_it it = kpw_sec_find(self->kpw, i);
		if (!i || KPW_IT_END(it)) {
			PyErr_Format(PyExc_IndexError,
				"type %lu not found", (unsigned long)i);
			return NULL;
		}
		return PyKpwsection_new(self, it);
	}
	else {
		PyErr_Format(PyExc_TypeError,
			"Array indices must be integers, not %.200s",
			item->ob_type->tp_name);
		return NULL;
	}
}


static PyMappingMethods kpwfile_as_mapping = {
	0,                              /* mp_length */
	(binaryfunc)kpwfile_subscript,  /* mp_subscript */
	0                               /* mp_ass_subscript */
};


/*** Type-Definitions ***/

static PyMemberDef PyKpwfile_members[];
static PyMethodDef PyKpwfile_methods[];

PyTypeObject PyKpwfile_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"kapow.KPWFile",             /*tp_name*/
	sizeof(PyKpwfile),          /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)PyKpwfile_dealloc,                         /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	&kpwfile_as_mapping,       /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"KAPOW KPW File",             /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	kpwfile_iter,                         /* tp_iter */
	0,                         /* tp_iternext */
	PyKpwfile_methods,          /* tp_methods */
	PyKpwfile_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)kpwfile_init,   /* tp_init */
	0,                         /* tp_alloc */
	kpwfile_new,              /* tp_new */
};

static PyMemberDef PyKpwfile_members[] = {
//	{"instvar", T_INT, offsetof(PyKpwfile, instvar), READONLY, "Instance variable ..."},
	{NULL}  /* Sentinel */
};

static PyMethodDef PyKpwfile_methods[] = {
	_PyMethod(kpwfile, list, METH_NOARGS),
	_PyMethod(kpwfile, load_stats, METH_NOARGS),
	_PyMethod(kpwfile, save_stats, METH_VARARGS),
	_PyMethod(kpwfile, load_trac, METH_NOARGS),
	_PyMethod(kpwfile, save_trac, METH_VARARGS),
	_PyMethod(kpwfile, load_lcp, METH_NOARGS),
	_PyMethod(kpwfile, save_lcp, METH_VARARGS),
	_PyMethod(kpwfile, load_lcpset, METH_NOARGS),
	_PyMethod(kpwfile, save_lcpset, METH_VARARGS),
	_PyMethod(kpwfile, load_bwt, METH_NOARGS),
	_PyMethod(kpwfile, save_bwt, METH_VARARGS),
	/*
	{"method", (PyCFunction)PyKpwfile_method, METH_VARARGS, method__doc__},
	*/
	{NULL}  /* Sentinel */
};

/* Type-Initialization */


void kapow_Kpwfile_init(PyObject* m) {

	PyKpwfile_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PyKpwfile_Type) < 0)
		return;

	Py_INCREF(&PyKpwfile_Type);
	PyModule_AddObject(m, "KPWFile", (PyObject *)&PyKpwfile_Type);

	/* Macros */
	PyModule_AddIntMacro(m, KPW_ID_EMPTY);
	//PyModule_AddIntMacro(m, KPW_MAX_SECTION_ID);
	PyModule_AddIntMacro(m, KPW_ID_STATS);
	PyModule_AddIntMacro(m, KPW_ID_BWT);
	PyModule_AddIntMacro(m, KPW_ID_TRAC);
	PyModule_AddIntMacro(m, KPW_ID_PATT);
	PyModule_AddIntMacro(m, KPW_ID_LCP);
	PyModule_AddIntMacro(m, KPW_ID_LCPS);

	KapowKpwfileError = PyErr_NewException("kapow.KapowKpwfileError", NULL, NULL);
	Py_INCREF(KapowKpwfileError);
}

