#ifndef __KSAINDEX_H__
#define __KSAINDEX_H__

#include "karray.h"
#include "saindex.h"

/* Tipos */

typedef struct str_PyKSAIndex {
	PyObject_HEAD
	PyKArray *ks; /* Encoded input */
	sa_index sa;
} PyKSAIndex;

PyAPI_DATA(PyTypeObject) PyKSAIndex_Type;

PyAPI_FUNC(PyObject *) PyKSAIndex_new(void);

/* Macros */

#define PyKSAIndex_Check(op) (Py_TYPE(op) == &PyKSAIndex_Type)

/* Init */

void kapow_SAIndex_init(PyObject* m);

#endif
