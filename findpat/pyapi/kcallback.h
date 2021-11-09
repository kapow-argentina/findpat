#ifndef __KCALLBACK_H__
#define __KCALLBACK_H__

#include "output_callbacks.h"

/* Tipos */

typedef struct str_PyKCallback PyKCallback;

union output_callback_data {
	output_readable_data readable_data;
};
typedef void(callback_stop)(void*);
typedef PyObject*(callback_get)(PyKCallback*);
typedef int (callback_init)(PyKCallback *self, PyObject* args);

struct str_PyKCallback {
	PyObject_HEAD
	PyObject* init_args;
	output_callback* cback;
	callback_init* cbinit;
	callback_get* cbget;
	callback_stop* cbstop;
	union output_callback_data data;
};

PyAPI_DATA(PyTypeObject) PyKCallback_Type;

PyAPI_FUNC(PyObject *) PyKCallback_new(void);
PyAPI_FUNC(int) PyKCallback_init_args(PyKCallback* self);
PyAPI_FUNC(void) PyKCallback_stop(PyKCallback* self);

/* Macros */

#define PyKCallback_Check(op) (Py_TYPE(op) == &PyKCallback_Type)
#define PyKCallback_FUNC(op) (((PyKCallback*)(op))->cback)
#define PyKCallback_DATA(op) ((void*)&(((PyKCallback*)(op))->data))

#define CALLBACK_READABLE        0
#define CALLBACK_READABLE_PO     1
#define CALLBACK_READABLE_TRAC   2
#define CALLBACK_PPZ_MRS         3

/* Init */

void kapow_Callback_init(PyObject* m);

#endif
