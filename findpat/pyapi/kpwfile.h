#ifndef __KPWFILE_H__
#define __KPWFILE_H__

#include "kpw.h"

/* Tipos */

typedef struct str_PyKpwfile {
	PyObject_HEAD
	kpw_file* kpw;
} PyKpwfile;

void kapow_Kpwfile_init(PyObject* m);

PyAPI_DATA(PyTypeObject) PyKpwfile_Type;

/* Macros */

#define PyKpwfile_Check(op) (Py_TYPE(op) == &PyKpwfile_Type)

#endif
