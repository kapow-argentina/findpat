#include <Python.h>
#include "structmember.h"
#include "k<obj>.h"
#include "kmacros.h"

#define debug_call(METH, self) debug("oo PyK<Obj>: %9s %p\n", METH, (self))

/* Errores de Python */
static PyObject *Kapow<Obj>Error;

/* Tipo PyK<Obj> */

PyTypeObject PyK<Obj>_Type;

/* Constructors / Destructors */

PyObject *
PyK<Obj>_new(void) {
	PyK<Obj> *self;

	self = (PyK<Obj> *)PyObject_GC_New(PyK<Obj>, &PyK<Obj>_Type);
	if (self == NULL)
		return PyErr_NoMemory();
	
	self->data = NULL;
	
	debug_call("NEW", self);
	
	return (PyObject *)self;
}

static PyObject *
k<obj>_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyK<Obj> *self;

	/*
	if (! PyArg_ParseTuple(args, "...", &...))
		return NULL;
	*/

	self = (PyK<Obj> *)PyK<Obj>_new();

	return (PyObject *)self;
}

int
PyK<Obj>_init(PyK<Obj> *self) {
	
	self->data = NULL;
	
	debug_call("INIT", self);
	
	return 0;
}

static int
k<obj>_init(PyK<Obj> *self, PyObject *args, PyObject *kwds) {
	/*
	if (! PyArg_ParseTuple(args, "...", &...))
		return -1;
	*/
	
	PyK<Obj>_init(self); // + args...
	
	return 0;
}


static void
k<obj>_dealloc(register PyK<Obj>* self) {
	debug_call("DEALLOC", self);
	PyObject_GC_UnTrack(self);        // must untrack first
/*
	Py_TRASHCAN_SAFE_BEGIN(self)
	... The body of the deallocator goes here, including all calls ...
	... to Py_DECREF on contained objects.                         ...
	Py_TRASHCAN_SAFE_END(self)
*/
	PyObject_GC_Del(self);
	// o esta otra?? Py_TYPE(self)->tp_free((PyObject *)self);
}

/*** Type-Definitions ***/

static PyMemberDef PyK<Obj>_members[];
static PyMethodDef PyK<Obj>_methods[];

PyTypeObject PyK<Obj>_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"kapow.<Obj>",             /*tp_name*/
	sizeof(PyK<Obj>),          /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)k<obj>_dealloc,                         /*tp_dealloc*/
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
	"KAPOW <Obj>",             /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PyK<Obj>_methods,          /* tp_methods */
	PyK<Obj>_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)k<obj>_init,     /* tp_init */
	0,                         /* tp_alloc */
	k<obj>_new,                /* tp_new */
};

static PyMemberDef PyK<Obj>_members[] = {
	/*
	{"instvar", T_INT, offsetof(PyK<Obj>, instvar), READONLY, "Instance variable ..."},
	*/
	{NULL}  /* Sentinel */
};

static PyMethodDef PyK<Obj>_methods[] = {
	/*
	{"method", (PyCFunction)PyK<Obj>_method, METH_VARARGS, method__doc__},
	_PyMethod(k<obj>, otromethod, METH_VARARGS),
	*/
	{NULL}  /* Sentinel */
};

/* Type-Initialization */


void kapow_<Obj>_init(PyObject* m) {

	PyK<Obj>_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PyK<Obj>_Type) < 0)
		return;

	Py_INCREF(&PyK<Obj>_Type);
	PyModule_AddObject(m, "<Obj>", (PyObject *)&PyK<Obj>_Type);

	Kapow<Obj>Error = PyErr_NewException("kapow.Kapow<Obj>Error", NULL, NULL);
	Py_INCREF(Kapow<Obj>Error);
}

