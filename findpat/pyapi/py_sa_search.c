/* Wrapers for functions from lcp.c */

#include "sa_search.h"
#include "fastq.h"

PyDoc_STRVAR(sa_search__doc__,
"sa_search(src, r, pat) -> from, to, time\n"
"\n"
"Searches the string or kapow.Array 'pat' in src using the suffix array\n"
"in r. src and r must have the same length.\n"
"Returns the range [from,to) of r which contains the given pattern.\n"
);

static PyObject *
kapow_sa_search(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *kr = NULL, *kpat = NULL;
	TIME_VARS;

	uint *r;
	uint n, pn, vfrom, vto;
	uchar *src, *pat;

	static char *kwlist[] = {"src", "r", "pat", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO!O:sa_search", kwlist, &ksrc, &PyKArray_Type, &kr, &kpat))
		return NULL;

	paramCheckKArrayStr(ksrc, src, n);
	paramCheckKArrayStr(kpat, pat, pn);

	uint fldsz = 4;
	paramCheckArraySize(kr, n, fldsz);

	r = PyKArray_GET_BUF(kr);

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	sa_search(src, r, n, pat, pn, &vfrom, &vto);
	TIME_END;

	Py_END_ALLOW_THREADS

	return Py_BuildValue("IId", vfrom, vto, TIME_DIFF);
}


PyDoc_STRVAR(sa_search_many__doc__,
"sa_search_many(src, r, pats, sep='\\000', from=None, to=None) -> total_count,time\n"
"\n"
"Searches each of the strings given in the string or kapow.Array 'pats'\n"
"delimiteds by a separator char 'sep', using the suffix array r of the\n"
"given src string or kapow.Array. src and r must have the same length.\n"
"If 'sep' is None, pats is assumed to be in FASTQ format.\n"
// "Returns the range [from,to) of r which contains the given pattern.\n"
);

static PyObject *
kapow_sa_search_many(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *kr = NULL, *kpat = NULL;
	PyObject* onone = NULL;
	int fastq = 0;
	TIME_VARS;

	uint *r;
	uint n, pn, vfrom, vto;
	uchar *src, *pat;
	uchar sep = 0;

	static char *kwlist[] = {"src", "r", "pats", "sep", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO!O|c:sa_search_many", kwlist, &ksrc, &PyKArray_Type, &kr, &kpat, &sep)) {
		PyErr_Clear();
		if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO!O|O:sa_search_many", kwlist, &ksrc, &PyKArray_Type, &kr, &kpat, &onone))
			return NULL;
		if (onone != Py_None) {
			PyErr_SetString(PyExc_TypeError, "sep must be a char or None");
			return NULL;
		}
		fastq = 1;
	}

	paramCheckKArrayStr(ksrc, src, n);
	paramCheckKArrayStr(kpat, pat, pn);

	uint fldsz = 4;
	paramCheckArraySize(kr, n, fldsz);

	r = PyKArray_GET_BUF(kr);
	unsigned long cnt = 0;

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	if (fastq) {
		for_fastq(it, pat, pn) {
			sa_search(src, r, n, fastq_it(it), fastq_len(it), &vfrom, &vto);
			cnt += (unsigned long)(vto-vfrom);
			// TODO: Save vfrom y vto
		}
	} else {
		for_line(it, pat, pn, sep) {
			sa_search(src, r, n, line_it(it), line_len(it), &vfrom, &vto);
			cnt += (unsigned long)(vto-vfrom);
			// TODO: Save vfrom y vto
		}
	}
	TIME_END;

	Py_END_ALLOW_THREADS

	return Py_BuildValue("kd", cnt, TIME_DIFF);
}


/*** Manber and Myers implementation ***/

PyDoc_STRVAR(sa_mm_init__doc__,
"sa_mm_search(h, llcp, rlcp, fldsz=None) -> time\n"
"\n"
"Precomputes the array Llcp and Rlcp to use with the suffix array\n"
"for the Manber and Myers algorithm.\n"
);
static PyObject *
kapow_sa_mm_init(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *kh = NULL, *kllcp = NULL, *krlcp = NULL;
	PyObject *kfldsz = NULL;
	TIME_VARS;

	uint *h, *llcp, *rlcp;
	uint n, sz=0;

	static char *kwlist[] = {"h", "llcp", "rlcp", "fldsz", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!O!O!|O:sa_mm_init", kwlist, &PyKArray_Type, &kh, &PyKArray_Type, &kllcp, &PyKArray_Type, &krlcp, &kfldsz))
		return NULL;
	if (kfldsz && kfldsz != Py_None && PyInt_Check(kfldsz)) {
		sz = PyInt_AS_LONG(kfldsz);
	}

	uint fldsz = 4;
	paramCheckArrayFldsz(kh, fldsz);
	n = PyKArray_GET_SIZE(kh);

	paramCheckArraySize(kllcp, n, fldsz);
	paramCheckArraySize(krlcp, n, fldsz);

	h = PyKArray_GET_BUF(kh);
	llcp = PyKArray_GET_BUF(kllcp);
	rlcp = PyKArray_GET_BUF(krlcp);

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	build_lrlcp(n, h, llcp, rlcp);
	TIME_END;

	Py_END_ALLOW_THREADS

	if (sz == 0) {
		unsigned PY_LONG_LONG mxl = PyKArray_max(kllcp);
		unsigned PY_LONG_LONG mxr = PyKArray_max(krlcp);
		// fprintf(stderr, "mxl=%llu, mxr=%llu\n", mxl, mxr);
		if (mxl<mxr) mxl = mxr;
		if (mxl <= 0xFFUL) sz = 1;
		else if (mxl <= 0xFFFFUL) sz = 2;
		else sz = 4;
	}
	if (sz == 1 || sz == 2) {
		PyKArray_resize(kllcp, n, sz);
		PyKArray_resize(krlcp, n, sz);
	}
	
	return Py_BuildValue("d", TIME_DIFF);
}

typedef bool(sa_mm_search_func)(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const void* llcp, const void* rlcp);

PyDoc_STRVAR(sa_mm_search__doc__,
"sa_mm_search(src, r, llcp, rlcp, pat) -> from, to, time\n"
"\n"
"Searches the string or kapow.Array 'pat' in src using the suffix array\n"
"in r, with the aditional arrays llcp and rlcp. It uses the algorithm\n"
"proposed by Manber and Myers (1993) that runs in O(|pat| + log(|src|)).\n"
"src, r, llcp and rlcp must have the same length.\n"
"Returns the range [from,to) of r which contains the given pattern.\n"
);

static PyObject *
kapow_sa_mm_search(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *kr = NULL, *kllcp = NULL, *krlcp = NULL, *kpat = NULL;
	TIME_VARS;

	uint *r;
	void *llcp, *rlcp;
	uint n, pn, vfrom, vto;
	uchar *src, *pat;
	sa_mm_search_func* func = NULL;

	static char *kwlist[] = {"src", "r", "llcp", "rlcp", "pat", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO!O!O!O:sa_mm_search", kwlist, &ksrc, &PyKArray_Type, &kr, &PyKArray_Type, &kllcp, &PyKArray_Type, &krlcp, &kpat))
		return NULL;

	paramCheckKArrayStr(ksrc, src, n);
	paramCheckKArrayStr(kpat, pat, pn);

	paramCheckArraySize(kr, n, 4);
	
	uint fldsz = PyKArray_GET_ITEMSZ(kllcp);
	if (fldsz != 1 && fldsz != 2 && fldsz != 4) {
		PyErr_SetString(PyExc_TypeError, "Invalid field size for llcp or rlcp.");
		return NULL;
	}

	paramCheckArraySize(kllcp, n, fldsz);
	paramCheckArraySize(krlcp, n, fldsz);

	if (fldsz == 1) func = (sa_mm_search_func*)sa_mm_search_8;
	else if (fldsz == 2) func = (sa_mm_search_func*)sa_mm_search_16;
	else if (fldsz == 4) func = (sa_mm_search_func*)sa_mm_search_32;

	r = PyKArray_GET_BUF(kr);
	llcp = PyKArray_GET_BUF(kllcp);
	rlcp = PyKArray_GET_BUF(krlcp);

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	func(src, r, n, pat, pn, &vfrom, &vto, llcp, rlcp);
	TIME_END;

	Py_END_ALLOW_THREADS

	return Py_BuildValue("IId", vfrom, vto, TIME_DIFF);
}


PyDoc_STRVAR(sa_mm_search_many__doc__,
"sa_mm_search_many(src, r, llcp, rlcp, pats, sep='\\000', from=None, to=None) -> total_count,time\n"
"\n"
"Searches each of the strings given in the string or kapow.Array 'pats'\n"
"delimiteds by a separator char 'sep', using the suffix array r of the\n"
"given src string or kapow.Array. src and r must have the same length.\n"
"If 'sep' is None, pats is assumed to be in FASTQ format.\n"
// "Returns the range [from,to) of r which contains the given pattern.\n"
);

static PyObject *
kapow_sa_mm_search_many(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *kr = NULL, *kllcp = NULL, *krlcp = NULL, *kpat = NULL, *afrom = NULL, *ato = NULL;
	PyObject* onone = NULL;
	int fastq = 0;
	TIME_VARS;

	uint *r;
	void *llcp, *rlcp;
	uint n, pn, vfrom, vto;
	uchar *src, *pat;
	uchar sep = 0;
	sa_mm_search_func* func = NULL;

	static char *kwlist[] = {"src", "r", "llcp", "rlcp", "pats", "sep", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO!O!O!O|c:sa_mm_search_many", kwlist, &ksrc, &PyKArray_Type, &kr, &PyKArray_Type, &kllcp, &PyKArray_Type, &krlcp, &kpat, &sep)) {
		PyErr_Clear();
		if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO!O!O!O|O:sa_mm_search_many", kwlist, &ksrc, &PyKArray_Type, &kr, &PyKArray_Type, &kllcp, &PyKArray_Type, &krlcp, &kpat, &onone))
			return NULL;
		if (onone != Py_None) {
			PyErr_SetString(PyExc_TypeError, "sep must be a char or None");
			return NULL;
		}
		fastq = 1;
	}

	paramCheckKArrayStr(ksrc, src, n);
	paramCheckKArrayStr(kpat, pat, pn);

	paramCheckArraySize(kr, n, 4);
	
	uint fldsz = PyKArray_GET_ITEMSZ(kllcp);
	if (fldsz != 1 && fldsz != 2 && fldsz != 4) {
		PyErr_SetString(PyExc_TypeError, "Invalid field size for llcp or rlcp.");
		return NULL;
	}
	paramCheckArraySize(kllcp, n, fldsz);
	paramCheckArraySize(krlcp, n, fldsz);

	if (fldsz == 1) func = (sa_mm_search_func*)sa_mm_search_8;
	else if (fldsz == 2) func = (sa_mm_search_func*)sa_mm_search_16;
	else if (fldsz == 4) func = (sa_mm_search_func*)sa_mm_search_32;

	r = PyKArray_GET_BUF(kr);
	llcp = PyKArray_GET_BUF(kllcp);
	rlcp = PyKArray_GET_BUF(krlcp);

	afrom = (PyKArray*)PyKArray_new();
	ato = (PyKArray*)PyKArray_new();

	unsigned long cnt = 0;
	unsigned long outsize = 0;

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	if (fastq) {
		for_fastq(it, pat, pn) outsize++;
	} else {
		for_line(it, pat, pn, sep) outsize++;
	}
	Py_END_ALLOW_THREADS
	
	uint *vlfrom, *vlto;
	if (PyKArray_resize(afrom, outsize, sizeof(*vlfrom)) < 0)
		return NULL;
	if (PyKArray_resize(ato, outsize, sizeof(*vlto)) < 0)
		return NULL;
	vlfrom = PyKArray_GET_BUF(afrom);
	vlto = PyKArray_GET_BUF(ato);

	Py_BEGIN_ALLOW_THREADS

	TIME_START;
	if (fastq) {
		for_fastq(it, pat, pn) {
			func(src, r, n, fastq_it(it), fastq_len(it), &vfrom, &vto, llcp, rlcp);
			cnt += (unsigned long)(vto-vfrom);
			*(vlfrom++) = vfrom;
			*(vlto++) = vto;
		}
	} else {
		for_line(it, pat, pn, sep) {
			func(src, r, n, line_it(it), line_len(it), &vfrom, &vto, llcp, rlcp);
			cnt += (unsigned long)(vto-vfrom);
			*(vlfrom++) = vfrom;
			*(vlto++) = vto;
		}
	}
	TIME_END;

	Py_END_ALLOW_THREADS

	return Py_BuildValue("NNkd", afrom, ato, cnt, TIME_DIFF);
}
