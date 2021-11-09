#ifndef __KRANKSEL_H__
#define __KRANKSEL_H__

#include "karray.h"
#include "ranksel.h"

/* Tipos */
typedef struct str_PyKRanksel {
	PyObject_HEAD

	PyKArray* array;
	unsigned long long int size_bits;
	ranksel *rks;
} PyKRanksel;

void kapow_Ranksel_init(PyObject* m);

PyAPI_DATA(PyTypeObject) PyKRanksel_Type;

#endif
