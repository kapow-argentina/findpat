#include <Python.h>
#include "structmember.h"
#include "krmq.h"
#include "kmacros.h"

#define debug_call(METH, self) debug("oo PyKRMQ: %9s %p, karr=%p\n", METH, (self), ((PyKRMQ*)self)->obj)

/* MACROS y definiones de variedades de RMQ */

typedef void (rmq_init_f)(rmq* q, uint* vec, uint n);
typedef uint (rmq_query_f)(const rmq* const q, uint a, uint b);
typedef void (rmq_update_f)(rmq* q, uint a, uint n);
typedef uint (rmq_nexteq_f)(rmq* q, uint i, uint vl, uint n);
typedef uint (rmq_preveq_f)(rmq* q, uint i, uint vl, uint n);

typedef struct str_rmq_function {
	rmq_init_f* init;
	rmq_query_f* query;
	rmq_update_f* update;
	rmq_nexteq_f* nexteq;
	rmq_preveq_f* preveq;
} rmq_function;

#define RMQ_MIN 0
#define RMQ_MAX 1
#define RMQ_TIPO_MAX 2

#define _def_rmq_f(op, one) {&rmq_init##op, &rmq_query##op, &rmq_update##op, &rmq_nexteq##one, &rmq_preveq##one}

static rmq_function rmq_functions[RMQ_TIPO_MAX] = {
	_def_rmq_f(_min, _leq),
	_def_rmq_f(_max, _geq)
};

/* Errores de Python */
static PyObject *KapowRmqError;

/* Tipo PyKRMQ */

PyTypeObject PyKRMQ_Type;

/* Constructors / Destructors */

PyObject *
PyKRMQ_new(void) {
	PyKRMQ *self;

	self = (PyKRMQ *)PyObject_GC_New(PyKRMQ, &PyKRMQ_Type);
	if (self == NULL)
		return PyErr_NoMemory();
	
	self->data = NULL;
	
	debug_call("NEW", self);
	
	return (PyObject *)self;
}

static PyObject *
krmq_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	PyKRMQ *self;

	/*
	if (! PyArg_ParseTuple(args, "...", &...))
		return NULL;
	*/

	self = (PyKRMQ *)PyKRMQ_new();

	return (PyObject *)self;
}

PyDoc_STRVAR(__init__doc__,
"KAPOW Range Minimum Query\n"
"\n"
"RMQ.__init__(tipo, arr)\n"
"\n"
"Implements a Range Minimum Query over an existing kapow.Array of ints.\n"
"If you want to modify the values of the Array, you need to call RMQ.set\n"
"or RMQ[] operator to update the internal structure.\n"
"The RMQ could be for Minimum Query (tipo==kapow.RMQ_MIN) or for Maximum\n"
"Query (tipo==kapow.RMQ_MAX).\n");


int
PyKRMQ_init(PyKRMQ *self, unsigned int tipo, PyKArray* obj) {
	debug_call("INIT", self);
	
	if (tipo >= RMQ_TIPO_MAX) {
		PyErr_SetString(KapowRmqError, "Invalid RMQ type");
		return -1;
	}

	if (obj && PyKArray_GET_SIZE(obj) && PyKArray_GET_ITEMSZ(obj) != 4) {
		PyErr_Format(PyExc_TypeError, "src must be a kapow.Array of elements of size %u.", 4);
		return -1;
	}

	/* Arma el RMQ */
	self->data = rmq_malloc(obj->n);
	if (!self->data) {
		PyErr_SetString(KapowRmqError, "Error allocing rmq");
		return -1;
	}
	
	self->obj = obj;
	Py_INCREF(obj);
	obj->binded_from++;
	self->tipo = tipo;
	(*(rmq_functions[self->tipo].init))(self->data, self->obj->buf, self->obj->n);
	
	return 0;
}

static int
krmq_init(PyKRMQ *self, PyObject *args, PyObject *kwds) {
	PyKArray* obj;
	unsigned int tipo;
	
	if (! PyArg_ParseTuple(args, "IO!", &tipo, &PyKArray_Type, &obj))
		return -1;
	
	return PyKRMQ_init(self, tipo, obj);
}


static void
krmq_dealloc(register PyKRMQ* self) {
	debug_call("DEALLOC", self);
	PyObject_GC_UnTrack(self);        // must untrack first
	
	if (self->data) {
		rmq_free(self->data, self->obj->n);
	}
	
	Py_TRASHCAN_SAFE_BEGIN(self)
	if (self->obj) {
		self->obj->binded_from--;
		Py_CLEAR(self->obj);
	}
	Py_TRASHCAN_SAFE_END(self)
	PyObject_GC_Del(self);
	// o esta otra?? Py_TYPE(self)->tp_free((PyObject *)self);
}



/*** Methods ***/

PyDoc_STRVAR(set__doc__,
"RMQ.set(pos, vl) -> ?\n"
"\n"
"Replaces the position 'pos' with vl.");

static PyObject *
krmq_set(PyKRMQ *self, PyObject *args, PyObject *kwds) {
	unsigned long i;
	uint vl;
	
	static char *kwlist[] = {"pos", "vl", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "kI|:set", kwlist, &i, &vl))
		return NULL;
	
	PyKArray_SET_ITEM(self->obj, i, vl);
	(*(rmq_functions[self->tipo].update))(self->data, i, self->obj->n);
	
	Py_RETURN_NONE;
}

PyDoc_STRVAR(query__doc__,
"RMQ.query(start, stop) -> ?\n"
"\n"
"Return the minimum (or maximum) in the intervale [i,j). If it's empty,\n"
"returns None.");

static PyObject *
krmq_query(PyKRMQ *self, PyObject *args, PyObject *kwds) {
	unsigned long i, j;
	
	static char *kwlist[] = {"start", "stop", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "kk|:query", kwlist, &i, &j))
		return NULL;
	
	if (i >= self->obj->n) i = self->obj->n -1;
	if (j > self->obj->n) j = self->obj->n;
	if (i >= j || !self->obj->n) {
		Py_RETURN_NONE;
	}
	return Py_BuildValue("K", (*(rmq_functions[self->tipo].query))(self->data, i, j));
}


PyDoc_STRVAR(nexteq__doc__,
"RMQ.nexteq(pos, cmpvl) -> ?\n"
"\n"
"Returns the first position greater than or equal to 'pos' that has a value of 'cmpvl'\n"
"...or less (for minimum query).\n"
"...or more (for maximum query).\n"
"If no such position exists returns None.\n"
"\n"
"Example:\n"
"	i = rmq.nexteq(0, cmpvl)\n"
"	while i != None:\n"
"		# Do something with i or rmq[i]\n"
"		i = rmq.nexteq(i+1, cmpvl)\n");

static PyObject *
krmq_nexteq(PyKRMQ *self, PyObject *args, PyObject *kwds) {
	unsigned long i, res;
	uint vl;
	
	static char *kwlist[] = {"pos", "cmpvl", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "kI|:nexteq", kwlist, &i, &vl))
		return NULL;
	
	if (i >= self->obj->n) {
		Py_RETURN_NONE;
	}
	res = (*(rmq_functions[self->tipo].nexteq))(self->data, i, vl, self->obj->n);
	if (res == self->obj->n) {
		Py_RETURN_NONE;
	}
	return Py_BuildValue("K", res);
}


PyDoc_STRVAR(preveq__doc__,
"RMQ.preveq(pos, cmpvl) -> ?\n"
"\n"
"Returns the first position lower than or equal to 'pos' that has a value of 'cmpvl'\n"
"...or less (for minimum query).\n"
"...or more (for maximum query).\n"
"If no such position exists returns None\n"
"\n"
"Example:\n"
"	i = rmq.preveq(len(rmq)-1, cmpval)\n"
"	while i != None:\n"
"		# Do something with i or rmq[i]\n"
"		i = rmq.preveq(i-1, cmpval)\n");

static PyObject *
krmq_preveq(PyKRMQ *self, PyObject *args, PyObject *kwds) {
	unsigned long i, res;
	uint vl;
	
	static char *kwlist[] = {"pos", "cmpvl", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "kI|:preveq", kwlist, &i, &vl))
		return NULL;
	
	if (i >= self->obj->n) {
		Py_RETURN_NONE;
	}
	res = (*(rmq_functions[self->tipo].preveq))(self->data, i, vl, self->obj->n);
	if (res == self->obj->n) {
		Py_RETURN_NONE;
	}
	return Py_BuildValue("K", res);
}


static Py_ssize_t
krmq_length(PyKRMQ *self) {
	return self->obj->n;
}
/**** Mapping functions (operator[]) ****/

static PyObject *
krmq_subscript(PyKRMQ* self, PyObject* item) {
	if (PyIndex_Check(item)) {
		Py_ssize_t i;
		i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return NULL;
		if (i < 0)
			i += PyKRMQ_GET_SIZE(self);
		if (i < 0 || i >= PyKRMQ_GET_SIZE(self)) {
			PyErr_Format(PyExc_IndexError,
				"index %lu out of range [0,%lu)", (unsigned long)i, (unsigned long)PyKRMQ_GET_SIZE(self));
			return NULL;
		}
		return Py_BuildValue("K", PyKArray_GET_ITEM(self->obj, i));
	}
	else if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelen;
		
		if (PySlice_GetIndicesEx((PySliceObject*)item, PyKRMQ_GET_SIZE(self), &start, &stop, &step, &slicelen) < 0)
			return NULL;
		if (step != 1) {
			PyErr_SetString(PyExc_TypeError, "RMQ indices slices must have a step of 1.");
			return NULL;
		}
		return Py_BuildValue("K", (*(rmq_functions[self->tipo].query))(self->data, start, start+slicelen));
	}
	else {
		PyErr_Format(PyExc_TypeError,
			"RMQ indices must be integers, not %.200s",
			item->ob_type->tp_name);
		return NULL;
	}
}

static int
krmq_ass_subscript(PyKRMQ* self, PyObject* item, PyObject* value) {
	if (PyIndex_Check(item)) {
		Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred())
			return -1;
		if (i < 0)
			i += PyKRMQ_GET_SIZE(self);
		if (i < 0 || i >= PyKRMQ_GET_SIZE(self)) {
			PyErr_Format(PyExc_IndexError,
				"index %lu out of range [0,%lu)", (unsigned long)i, (unsigned long)PyKRMQ_GET_SIZE(self));
			return -1;
		}
		if (PyInt_Check(value)) {
			unsigned PY_LONG_LONG vl = PyInt_AsUnsignedLongLongMask(value);
			PyKArray_SET_ITEM(self->obj, i, vl);
			(*(rmq_functions[self->tipo].update))(self->data, i, self->obj->n);
		} else if (PyString_Check(value) && PyString_GET_SIZE(value) == 1) {
			unsigned PY_LONG_LONG vl = *((unsigned char*)PyString_AS_STRING(value));
			PyKArray_SET_ITEM(self->obj, i, vl);
			(*(rmq_functions[self->tipo].update))(self->data, i, self->obj->n);
		} else {
			PyErr_Format(PyExc_TypeError,
				"RMQ values must be integers or chars, not %.200s",
				value->ob_type->tp_name);
			return -1;
		}
		return 0;
	}
	else {
		PyErr_Format(PyExc_TypeError,
			"RMQ indices must be integers, not %.200s",
			item->ob_type->tp_name);
		return -1;
	}
}

static PyMappingMethods krmq_as_mapping = {
	(lenfunc)krmq_length,
	(binaryfunc)krmq_subscript,
	(objobjargproc)krmq_ass_subscript
};

static int
krmq_traverse(PyKRMQ *self, visitproc visit, void *arg) {
	if (self->obj) {
		Py_VISIT(self->obj);
	}
	return 0;
}


/*** Type-Definitions ***/

static PyMemberDef PyKRMQ_members[];
static PyMethodDef PyKRMQ_methods[];

PyTypeObject PyKRMQ_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"kapow.RMQ",             /*tp_name*/
	sizeof(PyKRMQ),          /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)krmq_dealloc,                         /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	&krmq_as_mapping,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,        /*tp_flags*/
	__init__doc__,             /* tp_doc */
	(traverseproc)krmq_traverse,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PyKRMQ_methods,          /* tp_methods */
	PyKRMQ_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)krmq_init,     /* tp_init */
	0,                         /* tp_alloc */
	krmq_new,                /* tp_new */
};

static PyMemberDef PyKRMQ_members[] = {
	{"tipo", T_UINT, offsetof(PyKRMQ, tipo), READONLY, "RMQ variant (Minimum, Maximum)"},
	{NULL}  /* Sentinel */
};

static PyMethodDef PyKRMQ_methods[] = {
	_PyMethod(krmq, set, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(krmq, query, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(krmq, nexteq, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(krmq, preveq, METH_VARARGS | METH_KEYWORDS),
	{NULL}  /* Sentinel */
};

/* Type-Initialization */

void kapow_Rmq_init(PyObject* m) {
	PyKRMQ_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PyKRMQ_Type) < 0)
		return;

	Py_INCREF(&PyKRMQ_Type);
	PyModule_AddObject(m, "RMQ", (PyObject *)&PyKRMQ_Type);
	PyModule_AddIntMacro(m, RMQ_MIN);
	PyModule_AddIntMacro(m, RMQ_MAX);

	KapowRmqError = PyErr_NewException("kapow.KapowRmqError", NULL, NULL);
	Py_INCREF(KapowRmqError);
}

