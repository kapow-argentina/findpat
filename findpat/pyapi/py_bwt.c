/* Wrapers for functions from bwt.c */

#include "bwt.h"

typedef void(bwt_func)(uchar *bwt, uint* p, uint* r, uchar* src, uint n, uint* prim);
typedef void(bwt_bc_func)(uint* pp, uint* rr, uchar* src, uint nn, uint* prim, uint* c);

static inline PyObject *
kapow_bwt_gen(PyObject *self, PyObject *args, PyObject *kwds, bwt_func* bwtf, bwt_bc_func* bwt_bcf);

static PyObject *
kapow_bwt_gen(PyObject *self, PyObject *args, PyObject *kwds, bwt_func* bwtf, bwt_bc_func* bwt_bcf) {
	PyObject *ksrc = NULL, *kobwt = NULL;
	PyKArray *kp = NULL, *kr = NULL;
	TIME_VARS;

	void *p, *r;
	uint n, prim;
	uchar *src, *dst;
	PyObject *res;

	static char *kwlist[] = {"src", "obwt", "p", "r", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO:bwt", kwlist, &ksrc, &kobwt, &kp, &kr))
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

	paramNoneToNull(kp);
	paramNoneToNull(kr);
	paramNoneToNull(kobwt);

	paramCheckNullOr(PyKArray, kp);
	paramCheckNullOr(PyKArray, kr);
	paramCheckNullOr(PyKArray, kobwt);

	if (kobwt && PyKArray_GET_SIZE(kobwt) && PyKArray_GET_ITEMSZ(kobwt) != 1) {
		PyErr_SetString(PyExc_TypeError, "If set, obwt must be a kapow.Array of chars.");
		return NULL;
	}

	unsigned int fldsz = 0;
	if (kp && !fldsz) fldsz = kp->fldsz;
	if (kr && !fldsz) fldsz = kr->fldsz;
	if (!fldsz) fldsz = 4;
	if ((kp && kp->fldsz != fldsz && kp->fldsz != 0) ||
	    (kr && kr->fldsz != fldsz && kr->fldsz != 0) ||
	    (fldsz < 4)) {
		PyErr_SetString(PyExc_TypeError, "If set, both p and r must have the same element size of at least 4.");
		return NULL;
	}

	paramCheckArraySize(kp, n, fldsz);
	paramCheckArraySize(kr, n, fldsz);
	paramCheckArraySize(kobwt, n, 1);

	paramArrayExistsOrNew(kp, n, fldsz);
	paramArrayExistsOrNew(kr, n, fldsz);

	Py_BEGIN_ALLOW_THREADS

	p = (uint*)PyKArray_GET_BUF(kp);
	r = (uint*)PyKArray_GET_BUF(kr);
	//fprintf(stderr, "«%s» %d\n", src, n);
	if (kobwt) {
		dst = (uchar*)PyKArray_GET_BUF(kobwt);
		debug("bwt\n");
		TIME_START;
		(*bwtf)(dst, p, r, src, n, &prim);
		TIME_END;
	} else {
		uint bc[256];
		debug("bwt_bc\n");
		TIME_START;
		(*bwt_bcf)(p, r, src, n, &prim, bc);
		TIME_END;
	}
	//fprintf(stderr, "«%s» %d\n", dst, prim);

	Py_END_ALLOW_THREADS

	//odst = PyString_FromStringAndSize(NULL, n);
	//dst = (uchar*)PyString_AS_STRING(odst);
	Py_DECREF(kp);
	Py_DECREF(kr);
	
	res = Py_BuildValue("id", prim, TIME_DIFF);
	return res;
}


PyDoc_STRVAR(bwt__doc__,
"bwt(src, obwt=None, p=None, r=None) -> int,time\n"
"\n"
"Computes the Burrows-Wheeler transform of the given src array.\n"
"* src could be a kapow.Array of chars or a string.\n"
"* obwt could be a kapow.Array of chars or None if ignored.\n"
"* p and r could be a kapow.Array of integers or None to ignore the\n"
"  result. If given, they will contain the p and r arrays of the\n"
"  suffix array.\n"
"\n"
"The return value is the index in the BWT-matrix of src.\n");
static PyObject *
kapow_bwt(PyObject *self, PyObject *args, PyObject *kwds) {
	return kapow_bwt_gen(self, args, kwds, bwt, bwt_bc);
}


PyDoc_STRVAR(obwt__doc__,
"obwt(src, obwt=None, p=None, r=None) -> int,time\n"
"\n"
"The same as bwt() (see kapow.bwt()) but uses an optimized version that\n"
"consumes an extra array.\n");

static PyObject *
kapow_obwt(PyObject *self, PyObject *args, PyObject *kwds) {
	return kapow_bwt_gen(self, args, kwds, obwt, obwt_bc);
}



PyDoc_STRVAR(bwt_pr__doc__,
"bwt_pr(bwt, prim, p, r, bc=None) -> time\n"
"\n"
"Given the output of the BWT of some stream of length n, computes the\n"
"two permutation arrays p and r. bwt is an array with the BWT and prim\n"
"is the index corresponding to the row of the original string in the\n"
"BWT matrix. This number is returned by functions like kapow.bwt().\n"
"\n"
"Aditionaly, if bc is suplied, it will contain the frequence of each\n"
"char in bwt, usefull for other functions.\n");

static PyObject *
kapow_bwt_pr(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *kbwt = NULL;
	PyKArray *kp = NULL, *kr = NULL, *kbc = NULL;
	TIME_VARS;

	uint *p, *r, *bc;
	uint n, prim=0;
	uchar *bwt;

	static char *kwlist[] = {"bwt", "prim", "p", "r", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!kO!O!|O:bwt", kwlist, &PyKArray_Type, &kbwt, &prim, &PyKArray_Type, &kp, &PyKArray_Type, &kr, &kbc))
		return NULL;

	n = PyKArray_GET_SIZE(kbwt);

	paramNoneToNull(kbc);
	paramCheckNullOr(PyKArray, kbc);

	int fldsz = 4;
	paramCheckArraySize(kbwt, n, 1);
	paramCheckArraySize(kp, n, fldsz);
	paramCheckArraySize(kr, n, fldsz);

	
	bwt = PyKArray_GET_BUF(kbwt);
	p = PyKArray_GET_BUF(kp);
	r = PyKArray_GET_BUF(kr);
	if (kbc) {
		paramCheckArraySize(kbc, 256, fldsz);
		bc = PyKArray_GET_BUF(kbc);
	} else {
		bc = pz_malloc(256 * sizeof(uint));
	}

	Py_BEGIN_ALLOW_THREADS
	TIME_START;
	bwt_pr(bwt, p, r, n, prim, bc);
	TIME_END;
	Py_END_ALLOW_THREADS

	if (!kbc) pz_free(bc);

	return Py_BuildValue("d", TIME_DIFF);
}


PyDoc_STRVAR(frequency__doc__,
"frequency(src) -> bc\n"
"\n"
"Given a kapow.Array of chars or a string returns a kapow.Array with the\n"
"frequency of each char.\n");

static PyObject *
kapow_frequency(PyObject *self, PyObject *args) {
	PyObject *ksrc = NULL;
	PyKArray *kbc;

	uchar *src;
	uint n, *bc;

	if (! PyArg_ParseTuple(args, "O:frequency", &ksrc))
		return NULL;

	// Checkeo
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

	kbc = (PyKArray*)PyKArray_new();
	PyKArray_init(kbc, 256, sizeof(uint));
	bc = PyKArray_GET_BUF(kbc);

	Py_BEGIN_ALLOW_THREADS
	memset(bc, 0, 256*sizeof(uint));
	while (n--) bc[*(src++)]++;
	Py_END_ALLOW_THREADS

	return Py_BuildValue("N", kbc);
}
