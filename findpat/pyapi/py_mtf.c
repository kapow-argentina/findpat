/* Wrapers for functions from mtf.c */

#include "mtf.h"

PyDoc_STRVAR(mtf__doc__,
"mtf(src, dst=src) -> zero_mtf,time\n"
"\n"
"Computes the Move-to-Front algorithm of the given src array into the\n"
"given dst array. The destination array could be the same as src,\n"
"an empty array or an array of the same length as src.\n"
);

static PyObject *
kapow_mtf(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *kdst = NULL;
	TIME_VARS;

	uint n;
	uchar *src, *dst;
	unsigned long z_mtf;

	static char *kwlist[] = {"src", "dst", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!:mtf", kwlist, &PyKArray_Type, &ksrc, &PyKArray_Type, &kdst))
		return NULL;

	if (!kdst) kdst = ksrc;

	paramCheckArrayFldsz(ksrc, 1);
	n = PyKArray_GET_SIZE(ksrc);
	
	paramCheckArraySize(kdst, n, 1);

	src = PyKArray_GET_BUF(ksrc);
	dst = PyKArray_GET_BUF(kdst);

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	z_mtf = mtf_log(src, dst, n);
	TIME_END;

	Py_END_ALLOW_THREADS

	return Py_BuildValue("kd", z_mtf, TIME_DIFF);
}


PyDoc_STRVAR(imtf__doc__,
"imtf(src, dst=src) -> time\n"
"\n"
"Computes the inverse of Move-to-Front algorithm of the given src array\n"
"into the given dst array. The destination array could be the same as src,\n"
"an empty array or an array of the same length as src.\n"
);

static PyObject *
kapow_imtf(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *kdst = NULL;
	TIME_VARS;

	uint n;
	uchar *src, *dst;

	static char *kwlist[] = {"src", "dst", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!:imtf", kwlist, &PyKArray_Type, &ksrc, &PyKArray_Type, &kdst))
		return NULL;

	if (!kdst) kdst = ksrc;

	paramCheckArrayFldsz(ksrc, 1);
	n = PyKArray_GET_SIZE(ksrc);
	
	paramCheckArraySize(kdst, n, 1);

	src = PyKArray_GET_BUF(ksrc);
	dst = PyKArray_GET_BUF(kdst);

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	imtf_bf(src, dst, n);
	TIME_END;

	Py_END_ALLOW_THREADS

	return Py_BuildValue("d", TIME_DIFF);
}
