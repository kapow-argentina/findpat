#include <Python.h>
#include "structmember.h"

#include "karray.h"
#include "kranksel.h"
#include "kmacros.h"

#define debug_call(METH, self) debug("oo PyKRanksel: %9s %p\n", METH, (self))

/* Prototipos */
PyTypeObject PyKRanksel_Type;

/* Errores de Python */
static PyObject *KapowRankselError;

/* Constructors / Destructors */

static PyObject *
PyKRanksel_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyKRanksel *self;

	self = (PyKRanksel *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->array = NULL;
		self->size_bits = 0;
		self->rks = NULL;
	}

	debug_call("NEW", self);

	return (PyObject *)self;
}

PyDoc_STRVAR(__init__doc__,
"KAPOW Rank/Select\n"
"\n"
"R.__init__(arr, size_bits)\n"
"\n"
"Implements the Rank/Select.\n");

static int
PyKRanksel_init(PyKRanksel *self, PyObject *args, PyObject *kwds) {
	PyKArray *karray;
	unsigned long long int size_bits = 0;

	if (!PyArg_ParseTuple(args, "O!|K:ranksel", &PyKArray_Type, &karray, &size_bits))
		return -1;

	if (karray == NULL || PyKArray_GET_ITEMSZ(karray) != 1) {
		PyErr_SetString(KapowRankselError, "The array must be a kapow.Array of elements of size 1.");
		return -1;
	}

	if (size_bits == 0) {
		size_bits = PyKArray_GET_SIZE(karray) * 8;
	} else if (PyKArray_GET_SIZE(karray) != (size_bits + 7) / 8) {
		PyErr_SetString(KapowRankselError, "The array must have (n+7)/8 bytes.");
		return -1;
	}

	self->array = karray;
	self->size_bits = size_bits;
	self->rks = ranksel_create(size_bits, PyKArray_GET_BUF(karray));
	Py_INCREF(self->array);

	debug_call("INIT", self);

	return 0;
}

/*** Methods ***/

PyDoc_STRVAR(get__doc__,
"R.get(pos) -> bit\n"
"\n"
"Return the bit at the position pos of the underlying array.\n");

static PyObject *
PyKRanksel_get(PyKRanksel *self, PyObject *args, PyObject *kwds) {
	unsigned long long int pos;
	if (!PyArg_ParseTuple(args, "K:ranksel", &pos))
		return NULL;
	if (pos >= self->size_bits) {
		PyErr_Format(PyExc_IndexError, "index %lu out of range [0,%lu)",
				(unsigned long int)pos, (unsigned long int)self->size_bits);
		return NULL;
	}
	return Py_BuildValue("i", ranksel_get(self->rks, pos));
}

PyDoc_STRVAR(rank0__doc__,
"R.rank0(pos) -> count\n"
"\n"
"Return the number of 0s present in the positions [0,pos) of the underlying array.\n");

static PyObject *
PyKRanksel_rank0(PyKRanksel *self, PyObject *args, PyObject *kwds) {
	unsigned long long int pos;
	if (!PyArg_ParseTuple(args, "K:ranksel", &pos))
		return NULL;
	if (pos > self->size_bits) {
		PyErr_Format(PyExc_IndexError, "index %lu out of range [0,%lu]",
				(unsigned long int)pos, (unsigned long int)self->size_bits);
		return NULL;
	}
	return Py_BuildValue("K", ranksel_rank0(self->rks, pos));
}

PyDoc_STRVAR(rank1__doc__,
"R.rank1(pos) -> count\n"
"\n"
"Return the number of 1s present in the positions [0,pos) of the underlying array.\n");

static PyObject *
PyKRanksel_rank1(PyKRanksel *self, PyObject *args, PyObject *kwds) {
	unsigned long long int pos;
	if (!PyArg_ParseTuple(args, "K:ranksel", &pos))
		return NULL;
	if (pos > self->size_bits) {
		PyErr_Format(PyExc_IndexError, "index %lu out of range [0,%lu]",
				(unsigned long int)pos, (unsigned long int)self->size_bits);
		return NULL;
	}
	return Py_BuildValue("K", ranksel_rank1(self->rks, pos));
}

PyDoc_STRVAR(select0__doc__,
"R.select0(i) -> pos\n"
"\n"
"Return the smallest position such that R.rank0(pos) = i.\n"
"If there are less than i 0s in the underlying array, return R.size_bits + 1.\n");

static PyObject *
PyKRanksel_select0(PyKRanksel *self, PyObject *args, PyObject *kwds) {
	unsigned long long int count;
	if (!PyArg_ParseTuple(args, "K:ranksel", &count))
		return NULL;
	if (count == 0) {
		return Py_BuildValue("K", 0);
	} else {
		return Py_BuildValue("K", ranksel_select0(self->rks, count) + 1);
	}
}

PyDoc_STRVAR(select1__doc__,
"R.select1(i) -> pos\n"
"\n"
"Return the smallest position such that R.rank1(pos) = i.\n"
"If there are less than i 1s in the underlying array, return R.size_bits + 1.\n");

static PyObject *
PyKRanksel_select1(PyKRanksel *self, PyObject *args, PyObject *kwds) {
	unsigned long long int count;
	if (!PyArg_ParseTuple(args, "K:ranksel", &count))
		return NULL;
	if (count == 0) {
		return Py_BuildValue("K", 0);
	} else {
		return Py_BuildValue("K", ranksel_select1(self->rks, count) + 1);
	}
}

static void
PyKRanksel_dealloc(PyKRanksel* self) {
	debug_call("DEALLOC", self);
	PyObject_GC_UnTrack(self);        // must untrack first
	
	if (self->rks) {
		ranksel_destroy(self->rks);
		self->rks = NULL;
	}
	
	Py_TRASHCAN_SAFE_BEGIN(self)
	if (self->array) {
		Py_CLEAR(self->array);
	}
	Py_TRASHCAN_SAFE_END(self)
	
	self->ob_type->tp_free((PyObject*)self);
}

static int
kranksel_traverse(PyKRanksel *self, visitproc visit, void *arg) {
	if (self->array) { Py_VISIT(self->array); }
	return 0;
}

/*** Type-Definitions ***/

static PyMemberDef PyKRanksel_members[];
static PyMethodDef PyKRanksel_methods[];

PyTypeObject PyKRanksel_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"kapow.Ranksel",           /*tp_name*/
	sizeof(PyKRanksel),        /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)PyKRanksel_dealloc,                         /*tp_dealloc*/
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
	(traverseproc)kranksel_traverse,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PyKRanksel_methods,        /* tp_methods */
	PyKRanksel_members,        /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PyKRanksel_init, /* tp_init */
	0,                         /* tp_alloc */
	PyKRanksel_new,            /* tp_new */
};

static PyMemberDef PyKRanksel_members[] = {
	{"array", T_OBJECT, offsetof(PyKRanksel, array), READONLY, "Underlying array"},
	{"size_bits", T_ULONGLONG, offsetof(PyKRanksel, size_bits), READONLY, "Size in bits of the underlying array"},
	{NULL}  /* Sentinel */
};

static PyMethodDef PyKRanksel_methods[] = {
	{"get", (PyCFunction)PyKRanksel_get, METH_VARARGS, get__doc__},
	{"rank0", (PyCFunction)PyKRanksel_rank0, METH_VARARGS, rank0__doc__},
	{"rank1", (PyCFunction)PyKRanksel_rank1, METH_VARARGS, rank1__doc__},
	{"select0", (PyCFunction)PyKRanksel_select0, METH_VARARGS, select0__doc__},
	{"select1", (PyCFunction)PyKRanksel_select1, METH_VARARGS, select1__doc__},
	{NULL}  /* Sentinel */
};

/* Type-Initialization */

void kapow_Ranksel_init(PyObject* m) {
	PyKRanksel_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PyKRanksel_Type) < 0)
		return;

	Py_INCREF(&PyKRanksel_Type);
	PyModule_AddObject(m, "Ranksel", (PyObject *)&PyKRanksel_Type);

	KapowRankselError = PyErr_NewException("kapow.KapowRankselError", NULL, NULL);
	Py_INCREF(KapowRankselError);
}

