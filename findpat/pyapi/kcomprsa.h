#ifndef __KCOMPRSA_H__
#define __KCOMPRSA_H__

#include "karray.h"
#include "comprsa.h"

/* Tipos */

typedef struct str_PyKComprsa {
	PyObject_HEAD
	PyKArray *ks;
	sa_compressed *comprsa;
} PyKComprsa;

PyAPI_DATA(PyTypeObject) PyKComprsa_Type;

PyAPI_FUNC(PyObject *) PyKComprsa_new(void);

/* Macros */

#define PyKComprsa_Check(op) (Py_TYPE(op) == &PyKComprsa_Type)

/* Init */

void kapow_Comprsa_init(PyObject* m);

#endif
