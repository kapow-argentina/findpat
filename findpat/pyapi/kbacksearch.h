#ifndef __KBACKSEARCH_H__
#define __KBACKSEARCH_H__

#include "backsearch.h"
#include "tipos.h"

/* Tipos */

typedef struct str_PyKBackSearch {
	PyObject_HEAD
	backsearch_data data;
	
} PyKBackSearch;

PyAPI_DATA(PyTypeObject) PyKBackSearch_Type;

PyAPI_FUNC(PyObject *) PyKBackSearch_new(void);

/* Macros */

#define PyKBackSearch_Check(op) (Py_TYPE(op) == &PyKBackSearch_Type)

/* Init */

void kapow_BackSearch_init(PyObject* m);

#endif
