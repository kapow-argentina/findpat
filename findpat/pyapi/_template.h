#ifndef __K<OBJ>_H__
#define __K<OBJ>_H__

/* Tipos */

typedef struct str_PyK<Obj> {
	PyObject_HEAD
	void *data;
} PyK<Obj>;

PyAPI_DATA(PyTypeObject) PyK<Obj>_Type;

PyAPI_FUNC(PyObject *) PyK<Obj>_new(void);

/* Macros */

#define PyK<Obj>_Check(op) (Py_TYPE(op) == &PyK<Obj>_Type)

/* Init */

void kapow_<Obj>_init(PyObject* m);

#endif
