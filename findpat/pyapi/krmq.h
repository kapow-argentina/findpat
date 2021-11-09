#ifndef __KRMQ_H__
#define __KRMQ_H__

#include "rmq.h"
#include "karray.h"

/* Tipos */

typedef struct str_PyKRMQ {
	PyObject_HEAD
	rmq *data;
	PyKArray* obj;
	int tipo;
} PyKRMQ;

PyAPI_DATA(PyTypeObject) PyKRMQ_Type;

PyAPI_FUNC(PyObject *) PyKRMQ_new(void);

/* Macros */

#define PyKRMQ_Check(op) (Py_TYPE(op) == &PyKRMQ_Type)
#define PyKRMQ_GET_SIZE(op) PyKArray_GET_SIZE(((PyKRMQ*)(op))->obj)

/* Init */

void kapow_Rmq_init(PyObject* m);

#endif
