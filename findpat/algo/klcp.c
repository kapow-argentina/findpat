#include <stdlib.h>
#include <math.h>
#include "klcp.h"
#include "sorters.h"

#include "macros.h"

/*** QSORT de indices sobre la estructura de 2 arrays ***/
#define _VAL(X) (*(X))
_def_qsort3(uint_qsort3, uint, uint, _VAL, <)
#undef _VAL

typedef long double tdbl;

long double klcp_lin(uint n, const uint* h) {
	uint i;
	long double res = 1.0;
	if (!n) return 0.0;
	forn(i, n-1) res += (long double)1.0 / (long double)(h[i] +1);
	return res;
}

long double klcp(uint n, uint* h) {
	long double res = 0.0;
	uint cnt = 0, i;
	if (!n) return 0.0;
	if (n==1) return 1.0;

	uint_qsort3(h, h+n-1);

	dforn(i, n-1) {
		cnt++;
		if (!i || h[i-1] != h[i]) {
			res += (long double)cnt / (long double)(h[i] + 1);
			cnt = 0;
		}
	}

	return res+1.0;
} 


/** LCP-Set encode **/
void lcp_settoarr(uchar* hset, uint* h, uint n) {
	uint i;
	uint vl = 0;
	forn(i, n-1) {
		*(h++) = vl += (*hset >> (i%8)) & 1; 
		if (i%8 == 7) hset++;
	}
}

void lcp_arrtoset_sort(uint* h, uchar* hset, uint n) {
	uint_qsort3(h, h+n-1);
	lcp_arrtoset(h, hset, n);
}

void lcp_arrtoset(uint* h, uchar* hset, uint n) {
	uint i;
	uint vl = 0;
	forn(i, n-1) {
		if (i%8 == 0) *hset = 0;
		*hset |= (*h != vl) << (i%8);
		vl = *(h++);
		if (i%8 == 7) hset++;
	}
}

long double klcp_f(uint n, uint* h, k_ind_func f) {
	long double res = 0.0;
	uint i;
	if (!n) return 0.0;
	if (n==1) return 1.0;

	uint_qsort3(h, h+n-1);

	dforn(i, n-1) {
		res += f(i, h[i]);
	}

	return res+1.0;	
}

long double klcpset_f(uint n, uchar* hset, k_ind_func f) {
	long double res = 0.0;
	uint i;
	uint vl = 0;
	if (!n) return 0.0;
	if (n==1) return 1.0;

	forn(i, n-1) {
		vl += (*hset >> (i%8)) & 1; 
		res += f(i, vl);
		if (i%8 == 7) hset++;
	}

	return res+1.0;	
}

void klcpset_f_arr(uint n, uchar* hset, long double* res) {
	uint i, j;
	uint vl = 0;
	uint l, l2;
	forn(j, KLCP_FUNCS) res[j] = 0.0;
	if (!n) return;

	/* definitions:
	 * l = lower_bound
	 * u = upper_bound
	 * i = current index
	 */

	l = l2 = -1;
	forn(i, n-1) {
		vl += (*hset >> (i%8)) & 1;
		if (!(i&(i+1))) l = ++l2/2; // Suma en las potencias de 2
		uint ll = l>vl?vl:l;
		const long double brj = 1.0/(vl-ll+1);
		res[0] += brj;
		res[1] += brj*brj;
		const long double norm = (long double)(i-vl)/(i-ll+1);
		res[2] += norm;
		res[3] += norm*norm;
		const long double inv = 1.0/(vl+1);
		res[4] += inv;
		res[5] += inv*inv;
		
		if (i%8 == 7) hset++;
	}

	forn(j, KLCP_FUNCS) res[j] += 1.0;
}


/*Inverse function, simply returns 1/(v+1) */ 
inline long double klcp_inverse(int i, int v) {
	return (long double)1.0 / (long double)(v+1);
}

static inline int lower_bound(int i) {
	return 0;
}

static inline int upper_bound(int i) {
	return i;
}

long double sub_lb(int i, int v) {
	return (long double)1.0 / (long double)(v-lower_bound(i)+1);
}

/* Normalize. Substract lower bound and normalizes uniformly to (0,1] based
 * on the upper bound */ 
inline long double klcp_normalize(int i, int v) {
	int u = upper_bound(i), l = lower_bound(i);
	return (long double)(u-v)/(u-l+1);
}


/* Normalize. Substract lower bound and normalizes uniformly to (0,1] based
 * on the upper bound */ 
static inline int bruijn_lower_bound(int i, int a) {
	return (int)floor(log((long double)i+1) / log((long double)a));
}

inline long double klcp_bruijn(int i, int v) {
	int l = bruijn_lower_bound(i, 4); /* FIXME: 4 is for ACGT */
	if (l > v) l = 0; // If the alphabet is larger than 4.
	return (long double)1.0/(v-l+1);
}

long double klcp_inverse2(int i, int v) { long double r = klcp_inverse(i, v); return r*r; } 
long double klcp_normalize2(int i, int v) { long double r = klcp_normalize(i, v); return r*r; } 
long double klcp_bruijn2(int i, int v) { long double r = klcp_bruijn(i, v); return r*r; } 


