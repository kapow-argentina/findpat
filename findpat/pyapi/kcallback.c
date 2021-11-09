#include <Python.h>
#include "structmember.h"
#include "kcallback.h"
#include "kmacros.h"

#include "karray.h"

#define debug_call(METH, self) debug("oo PyKCallback: %9s %p\n", METH, (self))

/*** The CALLBACKS ***/

// output_readable y output_readable_po
static int output_readable_init(PyKCallback *self, PyObject* args) {
	PyFileObject* fp;
	PyKArray *ks, *kr;
	if (!PyArg_ParseTuple(args, "O!O!O!:output_readable_init", &PyFile_Type, &fp, &PyKArray_Type, &ks, &PyKArray_Type, &kr))
		return -1;
	self->data.readable_data.fp = fp->f_fp;
	self->data.readable_data.s = ks->buf;
	self->data.readable_data.r = kr->buf;
	return 0;
}

static
PyObject* output_readable_get(PyKCallback* self) {
	if (self->init_args) {
		Py_INCREF(self->init_args);
		return self->init_args;
	}
	Py_RETURN_NONE;
}

/* Errores de Python */
static PyObject *KapowCallbackError;

/* Tipo PyKCallback */

PyTypeObject PyKCallback_Type;

/* Constructors / Destructors */

PyObject *
PyKCallback_new(void) {
	PyKCallback *self;

	self = (PyKCallback *)PyObject_GC_New(PyKCallback, &PyKCallback_Type);
	if (self == NULL)
		return PyErr_NoMemory();
	
	self->cback = NULL;
	self->cbget = NULL;
	self->cbstop = NULL;
	self->init_args = NULL;
	
	debug_call("NEW", self);
	
	return (PyObject *)self;
}

static PyObject *
kcallback_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyKCallback *self;

	self = (PyKCallback *)PyKCallback_new();

	return (PyObject *)self;
}

int
PyKCallback_init(PyKCallback *self, int fid, PyObject* tup) {
	#define _kcallback_from(typ, _cback, _cbstop, _init, _cbget) if ((typ) == fid) { \
		self->cback = (_cback); self->cbstop = (_cbstop); self->cbget = (_cbget); self->cbinit = (_init); \
	}
	_kcallback_from(CALLBACK_READABLE, output_readable, NULL, output_readable_init, output_readable_get) else
	_kcallback_from(CALLBACK_READABLE_PO, output_readable_po, NULL, output_readable_init, output_readable_get) else
	{
		PyErr_Format(KapowCallbackError, "The given callback function is unknown.");
		return -1;
	}
	#undef _kcallback_from
	
	self->init_args = tup;
	Py_INCREF(tup);
	
	debug_call("INIT", self);
	
	return 0;
}

int PyKCallback_init_args(PyKCallback* self) {
	if (!self->cbinit) return 0;
	return (*(self->cbinit))(self, self->init_args);
}

void PyKCallback_stop(PyKCallback* self) {
	if (self->cbstop) (*(self->cbstop))(&(self->data));
}

static int
kcallback_init(PyKCallback *self, PyObject *args, PyObject *kwds) {
	PyObject* tup = Py_None;
	int fid;

	static char *kwlist[] = {"id", "arg", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "i|O:init", kwlist, &fid, &tup))
		return -1;
	
	PyKCallback_init(self, fid, tup);
	
	return 0;
}


static void
kcallback_dealloc(register PyKCallback* self) {
	debug_call("DEALLOC", self);
	PyObject_GC_UnTrack(self);        // must untrack first

	Py_TRASHCAN_SAFE_BEGIN(self)
	if (self->init_args) { Py_CLEAR(self->init_args); }
	Py_TRASHCAN_SAFE_END(self)
	PyObject_GC_Del(self);
	// o esta otra?? Py_TYPE(self)->tp_free((PyObject *)self);
}

/*** Methods ***/

PyDoc_STRVAR(data__doc__,
"CB.data() -> ?\n"
"\n"
"Returns the current state of the data asociated with the callback\n"
"function or None. This depends on the callback choosed.");

static PyObject *
kcallback_data(PyKCallback *self) {
	if (!self->cbget) {
		Py_RETURN_NONE;
	}
	return (*(self->cbget))(self);
}


/*** Type-Definitions ***/

PyDoc_STRVAR(__init__doc__,
"KAPOW Callback wraper\n"
"\n"
"Represents a builtin callback function for algorithms that uses an\n"
"output_callback like mrs(), mmrs(), etc. While a callable python object\n"
"can be passed to that functions, the overhead of calling many times\n"
"such objects could be negligible. For that reason, some builtin functions\n"
"are implemented here.\n"
"\n"
"Callback(id, arg) where id the selected function and arg is a tuple with\n"
"the initialization parameters:\n"
"  CALLBACK_READABLE: arg=(file, s, r)\n"
"    Writes to the File object 'file' each repeat and each occurence.\n"
"  CALLBACK_READABLE_PO: arg=(file, s, r)\n"
"    Writes to the File object 'file' each repeat, one per line.\n"
);

static PyMemberDef PyKCallback_members[];
static PyMethodDef PyKCallback_methods[];

PyTypeObject PyKCallback_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"kapow.Callback",             /*tp_name*/
	sizeof(PyKCallback),          /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)kcallback_dealloc,                         /*tp_dealloc*/
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
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PyKCallback_methods,          /* tp_methods */
	PyKCallback_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)kcallback_init,     /* tp_init */
	0,                         /* tp_alloc */
	kcallback_new,                /* tp_new */
};

static PyMemberDef PyKCallback_members[] = {
	/*
	{"instvar", T_INT, offsetof(PyKCallback, instvar), READONLY, "Instance variable ..."},
	*/
	{NULL}  /* Sentinel */
};

static PyMethodDef PyKCallback_methods[] = {
	/*
	{"data", (PyCFunction)PyKCallback_method, METH_VARARGS, method__doc__},
	*/
	_PyMethod(kcallback, data, METH_NOARGS),
	{NULL}  /* Sentinel */
};

/* Type-Initialization */


void kapow_Callback_init(PyObject* m) {

	PyKCallback_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PyKCallback_Type) < 0)
		return;

	Py_INCREF(&PyKCallback_Type);
	PyModule_AddObject(m, "Callback", (PyObject *)&PyKCallback_Type);

	PyModule_AddIntMacro(m, CALLBACK_READABLE);
	PyModule_AddIntMacro(m, CALLBACK_READABLE_PO);
	PyModule_AddIntMacro(m, CALLBACK_READABLE_TRAC);
	PyModule_AddIntMacro(m, CALLBACK_PPZ_MRS);

	KapowCallbackError = PyErr_NewException("kapow.KapowCallbackError", NULL, NULL);
	Py_INCREF(KapowCallbackError);
}

