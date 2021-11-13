#ifndef __SORTERS_H__
#define __SORTERS_H__

#define SWAP_T(tipo, a,b) { const tipo _tmp = *(a); *(a) = *(b); *(b)=_tmp;}

/** QSORT que parte el arreglo en tres partes, una con los <pivote, el pivote, >=pivote
 * Ordena elementos de tipo 'tipo' comparando valores VL(tipo*) de tipo 'tipoval'. */

#define _def_qsort(nombre, tipo, tipoval, VL, OP) \
void nombre(tipo* b, tipo* e) { \
/*	fprintf(stderr, "[%lX, %lX)  %s\n", (long unsigned int)b, (long unsigned int)e, b); */ \
	tipo *bp, *ep; \
	tipoval vp; \
	if (b >= e-1) return; \
	bp = b; \
	ep = e-1; \
	vp = (VL(b)); bp++; /* pivote */\
	while (bp < ep) { \
		while ((bp < ep) && ((VL(bp)) OP vp)) ++bp; \
		while ((bp < ep) && !((VL(ep)) OP vp)) --ep; \
		if (bp != ep) { SWAP_T(tipo, bp,ep); bp++; } \
	} \
	if ((VL(bp)) OP vp) { SWAP_T(tipo, b, bp); ep++; } else { --bp; if (b != bp) SWAP_T(tipo, b, bp); } \
	nombre(b,bp); \
	nombre(ep,e); \
}


/** QSORT que parte el arreglo en tres partes: <pivote, == pivote, > pivote */
//#define _qshow(X) { uchar* p = b; fprintf(stderr, "%s[%lX, %lX, %lX)  vp=%c  ", X, (long unsigned int)bp, (long unsigned int)cp, (long unsigned int)ep, vp); for(p=b;p<e;++p) fprintf(stderr, "%c", *p); fprintf(stderr, "\n"); }
//#define _qshow(X) { uint64* p = b; fprintf(stderr, "%s[%lX, %lX, %lX)  vp=%3d  ", X, (long unsigned int)bp, (long unsigned int)cp, (long unsigned int)ep, (int)vp); for(p=b;p<e;++p) fprintf(stderr, "%d ", (int)*p); fprintf(stderr, "\n"); }
#define _qshow(X)

#define _def_qsort3(nombre, tipo, tipoval, VL, OP) \
void nombre(tipo* b, tipo* e) { \
	tipo *bp, *ep, *cp; \
	tipoval vp, vcp; \
	if (b >= e-1) return; \
	cp = b+(rand()%(e-b)); \
	vp = (VL(cp)); /* pivote */\
	bp = cp = b; \
	ep = e; \
	_qshow(">> ") \
	while ((cp < ep) && (vp OP (VL((ep-1))))) --ep; \
	while(cp < ep) { \
		if ((vcp = (VL(cp))) OP vp) { \
			if (bp != cp) { SWAP_T(tipo, bp, cp); } ++bp; ++cp; \
		} else if (vcp == vp) { \
			++cp; \
		} else { \
			ep--; if (cp != ep) SWAP_T(tipo, cp, ep); \
			while ((cp < ep) && (vp OP (VL((ep-1))))) --ep;\
		} \
	} \
	_qshow("-- ") \
	nombre(b,bp); \
	nombre(ep,e); \
	_qshow("<< ") \
}

/** Sample sorters:
 *
 * defines: qsort_uint(uint* b, uint* e); using *x as value and < as comparator
#define _VAL(x) *x
_def_qsort(uint, uint, _VAL, <)

 *
 */

#endif
