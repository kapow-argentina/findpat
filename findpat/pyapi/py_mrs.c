/* Wrapers for functions from mrs.c and mrs_nolcp.c */

#include "mrs.h"
#include "mrs_nolcp.h"
#include "output_callbacks.h"


// Dummy callback
static void py_cback_none(uint l, uint i, uint n, void* obj) {}

typedef struct str_py_callable_data {
	PyObject* cback;
	PyObject* data;
} py_callable_data;
// Callback to call a Python "callable" object
static void py_cback_callable(uint l, uint i, uint n, void* obj) {
	debug("py_cback_callable %u %u %u %p {cback:%s at %p, data:%s at %p}\n", l, i, n, obj, Py_TYPE(((py_callable_data*)obj)->cback)->tp_name, ((py_callable_data*)obj)->cback, Py_TYPE(((py_callable_data*)obj)->data)->tp_name, ((py_callable_data*)obj)->data);
	PyObject* tuple = Py_BuildValue("IIIO", l, i, n, ((py_callable_data*)obj)->data);
	PyObject* res = PyObject_CallObject(((py_callable_data*)obj)->cback, tuple);
	if (res) Py_DECREF(res);
	Py_DECREF(tuple);
}


typedef void (mrs_func)(uchar* s, uint n, uint* r, uint* h, uint* p, uint ml, output_callback out, void* data);

static PyObject *
kapow_mrs_gen(PyObject *self, PyObject *args, PyObject *kwds, mrs_func* mrsf) {
	PyKArray *ksrc = NULL, *kh = NULL, *kp = NULL, *kr = NULL;
	PyObject *cback = Py_None, *data = Py_None;
	TIME_VARS;

	unsigned long ml = 1, n;
	uchar *src; uint *r, *h, *p;
	output_callback* v_cback; void* v_data;
	py_callable_data call_data;
	int allow_thread = 1;

	static char *kwlist[] = {"src", "r", "h", "p", "ml", "cback", "data", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO!O!O!|kOO:mrs", kwlist, &ksrc, &PyKArray_Type, &kr, &PyKArray_Type, &kh, &PyKArray_Type, &kp, &ml, &cback, &data))
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

	if (cback == Py_None) {
		v_cback = py_cback_none;
		v_data = NULL;
	} else if (PyKCallback_Check(cback)) {
		v_cback = PyKCallback_FUNC(cback);
		v_data = PyKCallback_DATA(cback);
	} else if (PyCallable_Check(cback)) {
		call_data.data = data;
		call_data.cback = cback;
		v_cback = py_cback_callable;
		v_data = &call_data;
		allow_thread = 0;
	} else {
		PyErr_Format(PyExc_TypeError, "cback must be None or callable, not %.200s.", Py_TYPE(cback)->tp_name);
		return NULL;
	}

	unsigned int fldsz = 4;
	paramCheckArraySize(kp, n, fldsz);
	paramCheckArraySize(kr, n, fldsz);
	paramCheckArraySize(kh, n, fldsz);

	r = (uint*)PyKArray_GET_BUF(kr);
	h = (uint*)PyKArray_GET_BUF(kh);
	p = (uint*)PyKArray_GET_BUF(kp);

	if (PyKCallback_Check(cback)) {
		if (PyKCallback_init_args((PyKCallback*)cback) < 0)
			return NULL;
	}

	if (allow_thread) {
		Py_BEGIN_ALLOW_THREADS
		TIME_START;
		mrsf(src, n, r, h, p, ml, v_cback, v_data);
		TIME_END;
		Py_END_ALLOW_THREADS
	} else {
		TIME_START;
		mrsf(src, n, r, h, p, ml, v_cback, v_data);
		TIME_END;
	}

	if (PyKCallback_Check(cback)) {
		PyKCallback_stop((PyKCallback*)cback);
	}

	return Py_BuildValue("d", TIME_DIFF);
}

PyDoc_STRVAR(mrs__doc__,
"mrs(src, r, h, p, ml=1, cback=None, data=None) -> time\n"
"\n"
"Calculates the maximal repeated substrings of the input string s of size n.\n"
"r, h and p must be of size n.\n"
"r must contain the starting point of all suffixes of s in lexicographical\n"
"order. h[i] must contain the length of the longest common prefix between\n"
"s[r[i]]s[r[i]+1]...s[n] and s[r[i+1]]s[r[i+1]+1]...s[n]. p may contain\n"
"anything, and will be returned with the inverse permutation of r.\n"
"ml is the minimum length a substring has to have to be considered\n"
"The output is defined by cback and data as follows:\n"
" * if cback is None, the output is just ignored (usefull for time measures).\n"
" * if cback is callable, then it's called for every repeat with in this way:\n"
"    cback(l, i, n, data), where:\n"
"   l is the lenght of the repeat\n"
"   i is the index of the position in the suffix array of the first suffix of\n"
"     the input string that has one of the substrings as a prefix.\n"
"   n is the number of such repetitions.\n"
"   data is the same object passed to mrs().\n"
);

static PyObject *
kapow_mrs(PyObject *self, PyObject *args, PyObject *kwds) {
	return kapow_mrs_gen(self, args, kwds, mrs);
}


PyDoc_STRVAR(mrs_nolcp__doc__,
"mrs_nolcp(src, r, h, p, ml=1, cback=None, data=None) -> time\n"
"\n"
"See kapow.mrs().\n"
);

static PyObject *
kapow_mrs_nolcp(PyObject *self, PyObject *args, PyObject *kwds) {
	return kapow_mrs_gen(self, args, kwds, mrs_nolcp);
}
