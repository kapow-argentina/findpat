/* Wrapers for functions from fastq.c */

/* Hey! There's no fastq.c! ... oh... that's true. It's implemented here. */

#include "fastq.h"

PyDoc_STRVAR(fastq_info__doc__,
"fastq_info(fastq) -> cant,avglen\n"
"\n"
"Counts the number of samples and average length in the 'fastq' input.\n"
);

static PyObject *
kapow_fastq_info(PyObject *self, PyObject *args) {
	PyKArray *kfastq;

	uchar *fq;
	unsigned long n, cnt=0, len=0;

	if (! PyArg_ParseTuple(args, "O!:fastq_info", &PyKArray_Type, &kfastq))
		return NULL;

	// Checkeos
	paramCheckKArrayStr(kfastq, fq, n);

	Py_BEGIN_ALLOW_THREADS

	for_fastq(it, fq, n) {
		cnt++;
		len += fastq_len(it);
	}

	Py_END_ALLOW_THREADS

	return Py_BuildValue("kd", cnt, (double)len / (double)cnt);
}

