/* Wrapers for functions from lcp.c */

#include "lcp.h"

PyDoc_STRVAR(lcp__doc__,
"lcp(src, h, p, r, sep=None) -> time\n"
"\n"
"Computes the Longest Common Prefix of the given src array.\n"
"* src could be a kapow.Array of chars or a string.\n"
"* h, p and r must be a kapow.Array of integers of the same length.\n"
"* sep, if given, is considered as a separator diferent to itself.\n"
);

static PyObject *
kapow_lcp(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *kh = NULL, *kp = NULL, *kr = NULL;
	TIME_VARS;

	void *h, *p, *r;
	uint n;
	uchar *src;
	PyObject *sep=NULL;
	uchar sp = 0;

	static char *kwlist[] = {"src", "h", "p", "r", "sep", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO!O!O!|O:lcp", kwlist, &ksrc, &PyKArray_Type, &kh, &PyKArray_Type, &kp, &PyKArray_Type, &kr, &sep))
		return NULL;

	// Checkeos
	if (ksrc && PyString_Check(ksrc)) {
		src = (uchar*)PyString_AS_STRING(ksrc);
		n = PyString_GET_SIZE(ksrc);
	} else if (ksrc && PyKArray_Check(ksrc)) {
		if (PyKArray_GET_ITEMSZ(ksrc) != 1) {
			PyErr_SetString(PyExc_TypeError, "src must be a kapow.Array of chars.");
			return NULL;
		}
		src = PyKArray_GET_BUF(ksrc);
		n = PyKArray_GET_SIZE(ksrc);
	} else {
		PyErr_SetString(PyExc_TypeError, "src must be a string or kapow.Array of chars.");
		return NULL;
	}

	paramNoneToNull(sep);
	paramCheckCharOrNull(sep, sp);

	uint fldsz = 4;
	// FIXME: Permitir arreglos h de n-1
	paramCheckArraySize(kh, n, fldsz);
	paramCheckArraySize(kp, n, fldsz);
	paramCheckArraySize(kr, n, fldsz);

	p = PyKArray_GET_BUF(kp);
	r = PyKArray_GET_BUF(kr);
	h = PyKArray_GET_BUF(kh);

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	if (sep) lcp3_sp(n, src, r, p, h, sp);
	else lcp3(n, src, r, p, h);
	TIME_END;
	if (kh->n >= n) PyKArray_SET_ITEM(kh, n-1, 0); // Llena el último int, si hay lugar

	Py_END_ALLOW_THREADS

	return Py_BuildValue("d", TIME_DIFF);
}
