/* Wrapers for functions from enc.c */

#include "enc.h"

PyDoc_STRVAR(fa_copy_cont_bind__doc__,
"fa_copy_cont_bind(A, sep=None) -> A2\n"
"\n"
"Given a FASTA stream in A removes the header and all non FASTA chars.\n"
"If the char sep is given, the occurences of sep in A are also copied.\n"
"Returns a new Array binded to the right portion of A. The content of A\n"
"is also modified, but not it's size.\n"
);

static PyObject *
kapow_fa_copy_cont_bind(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *other = NULL;

	unsigned long n, m;
	PyObject* sep = NULL;
	uchar *src, vsep = 'N';

	static char *kwlist[] = {"src", "sep", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!|O:fa_copy_cont", kwlist, &PyKArray_Type, &ksrc, &sep))
		return NULL;

	paramNoneToNull(sep);
	paramCheckCharOrNull(sep, vsep);

	paramCheckArrayFldsz(ksrc, 1);
	n = PyKArray_GET_SIZE(ksrc);
	src = PyKArray_GET_BUF(ksrc);
	
	other = (PyKArray*)PyKArray_new();

	Py_BEGIN_ALLOW_THREADS
	m = fa_copy_cont_sep(src, src, n, vsep);
	Py_END_ALLOW_THREADS
	
	/* Esta llamada no falla */
	Py_INCREF(ksrc);
	PyKArray_bindto(other, (PyObject*)ksrc, src, m, 1);
	return (PyObject*)other;
}


PyDoc_STRVAR(fa_copy_cont__doc__,
"fa_copy_cont(src, dst, sep=None) -> int\n"
"\n"
"Given a FASTA stream in src removes the header and all non FASTA chars.\n"
"If the char sep is given, the occurences of sep in src are also copied.\n"
"Returns the number of chars copied to dst. dst must have space for at\n"
"at least src.size chars.\n"
);

static PyObject *
kapow_fa_copy_cont(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *kdst = NULL;

	unsigned long n, m;
	PyObject* sep = NULL;
	uchar *src, *dst, vsep = 'N';

	static char *kwlist[] = {"src", "dst", "sep", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!O!|O:fa_copy_cont", kwlist, &PyKArray_Type, &ksrc, &PyKArray_Type, &kdst, &sep))
		return NULL;

	paramNoneToNull(sep);
	paramCheckCharOrNull(sep, vsep);

	paramCheckArrayFldsz(ksrc, 1);
	paramCheckArrayFldsz(kdst, 1);
	n = PyKArray_GET_SIZE(ksrc);
	m = PyKArray_GET_SIZE(kdst);
	if (!m) {
		if (PyKArray_resize(kdst, n, 1) < 0)
			return NULL;
		m = n;
	}
	if (n > m) {
		PyErr_SetString(PyExc_TypeError, "dst doesn't have enough space for the resulting array");
		return NULL;
	}
	src = PyKArray_GET_BUF(ksrc);
	dst = PyKArray_GET_BUF(kdst);

	Py_BEGIN_ALLOW_THREADS
	m = fa_copy_cont_sep(src, dst, n, vsep);
	Py_END_ALLOW_THREADS
	
	return Py_BuildValue("k", m);
}


PyDoc_STRVAR(fa_strip_n_bind__doc__,
"fa_strip_n_bind(src, trac=None, lres=0) -> A2,int\n"
"\n"
"Given a FASTA stream in src replaces long lists of Ns with a mark of\n"
"some Ns. Returns a binded array to the corresponding portion of src.\n"
"If trac is given, its augmented with the information needed to recover\n"
"the original positions.\n"
"\n"
"If you want to chain some FASTAs files, and trac the resulting stream,\n"
"you should use the returned int as lres for the next call.\n"
);

static PyObject *
kapow_fa_strip_n_bind(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL, *other = NULL, *ktrac = NULL;

	unsigned long n, m, lres = 0;
	uchar *src;
	uint trac_size=0;

	static char *kwlist[] = {"src", "trac", "lres", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!|Ok:fa_strip_n_bind", kwlist, &PyKArray_Type, &ksrc, &ktrac, &lres))
		return NULL;

	paramNoneToNull(ktrac);
	paramCheckArrayFldsz(ktrac, 4);
	paramCheckArrayFldsz(ksrc, 1);
	
	if (ktrac) trac_size = ktrac->n / 2;
	n = PyKArray_GET_SIZE(ksrc);
	src = PyKArray_GET_BUF(ksrc);

	other = (PyKArray*)PyKArray_new();
	if (!other)
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	if (ktrac) {
		m = fa_strip_n_trac(src, src, n, (uint**)&(ktrac->buf), &trac_size, lres);
	} else {
		m = fa_strip_n(src, src, n);
	}
	Py_END_ALLOW_THREADS
	if (ktrac) {
		ktrac->n = trac_size*2;
		ktrac->fldsz = 4;
	}
	
	/* Esta llamada no falla */
	Py_INCREF(ksrc);
	PyKArray_bindto(other, (PyObject*)ksrc, src, m, 1);
	return Py_BuildValue("Nk", other, m);
}


PyDoc_STRVAR(trac_virt_to_real__doc__,
"trac_virt_to_real(trac, virt) -> int\n"
"\n"
"Given the trac information of a FASTA stream, converts the virtual\n"
"position virt to the corresponding original position and returns it.\n"
);

static PyObject *
kapow_trac_virt_to_real(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ktrac = NULL;

	unsigned long v;

	static char *kwlist[] = {"trac", "virt", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!k:trac_virt_to_real", kwlist, &PyKArray_Type, &ktrac, &v))
		return NULL;

	paramCheckArrayFldsz(ktrac, 4);
	v = trac_convert_pos_virtual_to_real(v, ktrac->buf, ktrac->n/2);
	return Py_BuildValue("k", v);
}

PyDoc_STRVAR(trac_real_to_virt__doc__,
"trac_real_to_virt(trac, real) -> int\n"
"\n"
"Given the trac information of a FASTA stream, converts the original\n"
"position 'real' to the corresponding virtual position and returns it.\n"
);

static PyObject *
kapow_trac_real_to_virt(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ktrac = NULL;

	unsigned long v;

	static char *kwlist[] = {"trac", "virt", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O!k:trac_real_to_virt", kwlist, &PyKArray_Type, &ktrac, &v))
		return NULL;

	paramCheckArrayFldsz(ktrac, 4);
	v = trac_convert_pos_real_to_virtual(v, ktrac->buf, ktrac->n/2);
	return Py_BuildValue("k", v);
}


PyDoc_STRVAR(rev_comp__doc__,
"rev_comp(A) -> A\n"
"\n"
"Given a FASTA stream in A computes the reversed-complentary over the \n"
"same array A. Return a new reference to the same object.\n"
);

static PyObject *
kapow_rev_comp(PyObject *self, PyObject *args, PyObject *kwds) {
	PyKArray *ksrc = NULL;

	unsigned long n;
	uchar *src;

	if (! PyArg_ParseTuple(args, "O!:rev_comp", &PyKArray_Type, &ksrc))
		return NULL;

	paramCheckArrayFldsz(ksrc, 1);
	n = PyKArray_GET_SIZE(ksrc);
	src = PyKArray_GET_BUF(ksrc);

	Py_BEGIN_ALLOW_THREADS
	rev_comp(src, n);
	Py_END_ALLOW_THREADS
	
	Py_INCREF(ksrc);
	return (PyObject*)ksrc;
}
