/* Wrapers for functions from arit_enc.c */

#include "arit_enc.h"

PyDoc_STRVAR(entropy__doc__,
"entropy(A) -> double\n"
"\n"
"Computes the computational entropy of the given array.\n"
);

static PyObject *
kapow_entropy(PyObject *self, PyObject *args) {
	PyKArray *karr = NULL;

	unsigned long n;
	unsigned int fldsz;
	void *buf;
	long double res = 0; /* Inicializada para que gcc no se queje, no hace falta. */

	if (! PyArg_ParseTuple(args, "O!:entropy", &PyKArray_Type, &karr))
		return NULL;

	fldsz = PyKArray_GET_ITEMSZ(karr);
	n = PyKArray_GET_SIZE(karr);
	buf = PyKArray_GET_BUF(karr);

	if (!n && !fldsz) fldsz = 1;
	if (fldsz != 1 && fldsz != 2) {
		PyErr_SetString(PyExc_TypeError, "Unsupported element size for entropy().");
		return NULL;
	}
	
	Py_BEGIN_ALLOW_THREADS
	if (fldsz == 1) res = entropy_char(buf, n);
	else if (fldsz == 2) res = entropy_short(buf, n);
	Py_END_ALLOW_THREADS

	return Py_BuildValue("d", (double)(res * n));
}

