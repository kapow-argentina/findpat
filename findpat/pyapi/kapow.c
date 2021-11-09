// kapow.c

#include <Python.h>
#include "kmacros.h"
#include "karray.h"
#include "kpwfile.h"
#include "kcallback.h"
#include "kranksel.h"
#include "kcomprsa.h"
#include "ksaindex.h"
#include "krmq.h"
#include "kbacksearch.h"

#include "macros.h"
#include "tiempos.h"

/* Funciones varias */

#define sqr(X) (X)*(X)
#define abssub(a,b) ((a)<(b)?(b)-(a):(a)-(b))

/* Errores de Python */
PyObject *KapowTypeError;

/* Wrappers en C para Python */
#include "py_bwt.c"
#include "py_lcp.c"
#include "py_mtf.c"
#include "py_klcp.c"
#include "py_enc.c"
#include "py_arit_enc.c"
#include "py_mrs.c"
#include "py_sa_search.c"
#include "py_fastq.c"

/* Initialization */

static PyMethodDef kapow_methods[] = {
	/* py_bwt.c */
	_PyMethod(kapow, bwt, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, obwt, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, bwt_pr, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, frequency, METH_VARARGS),
	/* py_lcp.c */
	_PyMethod(kapow, lcp, METH_VARARGS | METH_KEYWORDS),
	/* py_mtf.c */
	_PyMethod(kapow, mtf, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, imtf, METH_VARARGS | METH_KEYWORDS),
	/* py_klcp.c */
	_PyMethod(kapow, klcp, METH_VARARGS),
	_PyMethod(kapow, klcp_lin, METH_VARARGS),
	_PyMethod(kapow, lcp_settoarr, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, lcp_arrtoset, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, lcp_arrtoset_sort, METH_VARARGS | METH_KEYWORDS),
	/* py_enc.c */
	_PyMethod(kapow, fa_copy_cont_bind, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, fa_copy_cont, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, fa_strip_n_bind, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, trac_virt_to_real, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, trac_real_to_virt, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, rev_comp, METH_VARARGS),
	/* py_arit_enc.c */
	_PyMethod(kapow, entropy, METH_VARARGS),
	/* py_mrs.c */
	_PyMethod(kapow, mrs, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, mrs_nolcp, METH_VARARGS | METH_KEYWORDS),
	/* py_sa_search.c */
	_PyMethod(kapow, sa_search, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, sa_search_many, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, sa_mm_init, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, sa_mm_search, METH_VARARGS | METH_KEYWORDS),
	_PyMethod(kapow, sa_mm_search_many, METH_VARARGS | METH_KEYWORDS),
	/* py_fastq.c */
	_PyMethod(kapow, fastq_info, METH_VARARGS),
	
	{NULL, NULL, 0, NULL}        /* Sentinel */
};


PyMODINIT_FUNC
initkapow(void) {
	PyObject *m;
	m = Py_InitModule("kapow", kapow_methods);
	if (!m) return;

	KapowTypeError = PyErr_NewException("kapow.KapowTypeError", NULL, NULL);
	Py_INCREF(KapowTypeError);
	PyModule_AddObject(m, "KapowTypeError", KapowTypeError);

	//PyObject* tools = PyImport_ImportModule("kapowtools");
	//PyModule_AddObject(m, "tools", tools);

	kapow_Array_init(m);
	kapow_Kpwfile_init(m);
	kapow_Callback_init(m);
	kapow_Ranksel_init(m);
	kapow_Rmq_init(m);
	kapow_BackSearch_init(m);
	kapow_Comprsa_init(m);
	kapow_SAIndex_init(m);
}
