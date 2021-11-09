/* Wrapers for functions from klcp.c */

#include "klcp.h"

PyDoc_STRVAR(klcp__doc__,
"klcp(h) -> double,time\n"
"\n"
"Computes the K function defined as 1+\\sum_{i=0}^{n-2}\\frac{1}{h[i]+1}\n"
"where h[i] is the LCP of a given string. Please note that the last\n"
"value of h (h[n-1]) is never used and assumed as 0.\n"
"Destroys the values of h.\n"
);

static PyObject *
kapow_klcp(PyObject *self, PyObject *args) {
	PyKArray *kh = NULL;
	TIME_VARS;

	uint n;
	uint *h;
	long double res;

	if (! PyArg_ParseTuple(args, "O!:klcp", &PyKArray_Type, &kh))
		return NULL;

	paramCheckArrayFldsz(kh, 4);
	n = PyKArray_GET_SIZE(kh);
	h = PyKArray_GET_BUF(kh);
	
	Py_BEGIN_ALLOW_THREADS
	TIME_START;
	res = klcp(n, h);
	TIME_END;
	Py_END_ALLOW_THREADS

	return Py_BuildValue("dd", (double)res, TIME_DIFF);
}

PyDoc_STRVAR(klcp_lin__doc__,
"klcp_lin(h) -> double,time\n"
"\n"
"Computes the K function (see klcp()) but don't destroy the values of h.\n"
);

static PyObject *
kapow_klcp_lin(PyObject *self, PyObject *args) {
	PyKArray *kh = NULL;
	TIME_VARS;

	unsigned long n;
	uint *h;
	long double res;

	if (! PyArg_ParseTuple(args, "O!:klcp_lin", &PyKArray_Type, &kh))
		return NULL;

	paramCheckArrayFldsz(kh, 4);
	n = PyKArray_GET_SIZE(kh);
	h = PyKArray_GET_BUF(kh);
	
	Py_BEGIN_ALLOW_THREADS
	TIME_START;
	res = klcp_lin(n, h);
	TIME_END;
	Py_END_ALLOW_THREADS

	return Py_BuildValue("dd", (double)res, TIME_DIFF);
}


PyDoc_STRVAR(lcp_settoarr__doc__,
"lcp_settoarr(hset, h) -> time\n"
"\n"
"Converts the source LCP set in bits (hset) to the destination array of\n"
"ints (h) sorted. h MUST have the correct size.\n"
);

static PyObject *
kapow_lcp_settoarr(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *kh = NULL, *khset = NULL;
	TIME_VARS;

	unsigned long n, nset;
	uint *h; uchar* hset;

	static char *kwlist[] = {"hset", "h", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!O!|:lcp_settoarr", kwlist, &PyKArray_Type, &khset, &PyKArray_Type, &kh))
		return NULL;

	paramCheckArrayFldsz(kh, 4);
	paramCheckArrayFldsz(khset, 1);
	n = PyKArray_GET_SIZE(kh);
	h = PyKArray_GET_BUF(kh);
	nset = PyKArray_GET_SIZE(khset);
	hset = PyKArray_GET_BUF(khset);
	if ((n+7) / 8 != nset) {
		PyErr_SetString(PyExc_TypeError, "h (in ints) and hset (in bits) must have a compatible size.");
		return NULL;
	}
	
	Py_BEGIN_ALLOW_THREADS
	TIME_START;
	lcp_settoarr(hset, h, n);
	TIME_END;
	Py_END_ALLOW_THREADS

	return Py_BuildValue("d", TIME_DIFF);
}


PyDoc_STRVAR(lcp_arrtoset__doc__,
"lcp_arrtoset(h, hset) -> time\n"
"\n"
"Converts the source LCP set ascending sorted in the array (h) to a\n"
"bit representation in hset. h MUST be sorted ascending.\n"
);

static PyObject *
kapow_lcp_arrtoset(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *kh = NULL, *khset = NULL;
	TIME_VARS;

	unsigned long n, nset;
	uint *h; uchar* hset;

	static char *kwlist[] = {"h", "hset", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!O!|:lcp_arrtoset", kwlist, &PyKArray_Type, &kh, &PyKArray_Type, &khset))
		return NULL;

	paramCheckArrayFldsz(kh, 4);
	n = PyKArray_GET_SIZE(kh);
	nset = (n+7)/8;
	paramCheckArraySize(khset, nset, 1);
	
	h = PyKArray_GET_BUF(kh);
	hset = PyKArray_GET_BUF(khset);
	
	Py_BEGIN_ALLOW_THREADS
	TIME_START;
	lcp_arrtoset(h, hset, n);
	TIME_END;
	Py_END_ALLOW_THREADS

	return Py_BuildValue("d", TIME_DIFF);
}


PyDoc_STRVAR(lcp_arrtoset_sort__doc__,
"lcp_arrtoset_sort(h, hset) -> time\n"
"\n"
"Converts the source LCP array (h) to a bit representation in hset.\n"
"The values in h are first sorted ascending.\n"
);

static PyObject *
kapow_lcp_arrtoset_sort(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *kh = NULL, *khset = NULL;
	TIME_VARS;

	unsigned long n, nset;
	uint *h; uchar* hset;

	static char *kwlist[] = {"h", "hset", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!O!|:lcp_arrtoset_sort", kwlist, &PyKArray_Type, &kh, &PyKArray_Type, &khset))
		return NULL;

	paramCheckArrayFldsz(kh, 4);
	n = PyKArray_GET_SIZE(kh);
	nset = (n+7)/8;
	paramCheckArraySize(khset, nset, 1);
	
	h = PyKArray_GET_BUF(kh);
	hset = PyKArray_GET_BUF(khset);
	
	Py_BEGIN_ALLOW_THREADS
	TIME_START;
	lcp_arrtoset_sort(h, hset, n);
	TIME_END;
	Py_END_ALLOW_THREADS

	return Py_BuildValue("d", TIME_DIFF);
}
