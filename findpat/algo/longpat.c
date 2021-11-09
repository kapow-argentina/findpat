#include "longpat.h"
#include "bittree.h"
#include "rmq.h"
#include "rmqbit.h"
#include <stdlib.h>
#include <string.h>

#include "macros.h"

/* This bittree answers:
 *
 * bittree_min_greater_than(bt, n1, i) == min{ j>i where bwto[j] != bwto[j-1] }, or n if no such j.

 */
bittree* longpat_bwttree(uchar *bwto, uint n) {
	uint i, n1 = n+1;
	bittree *bt = bittree_malloc(n+1);
	bittree_clear(bt, n1);
	bittree_preset(bt,n1,0);
	bittree_preset(bt,n1,n);
	forsn(i, 1, n) if (bwto[i-1] != bwto[i]) bittree_preset(bt,n,i);
	bittree_endset(bt,n1);
	return bt;
}

void longpat_bwttree_free(bittree* bwtt, uint n) {
	bittree_free(bwtt, n+1);
}

#define _min(a, b) ((a)<(b)?(a):(b))
#define _max(a, b) ((a)>(b)?(a):(b))

typedef rmqbit* (rmqbit_malloc_t)(uint n);
typedef void (rmqbit_free_t)(rmqbit* q, uint n);
typedef void (rmqbit_init_t)(rmqbit* q, uint* vec, uint n);
typedef uint (rmqbit_query_t)(const rmqbit* const q, uint a, uint b);
typedef void (rmqbit_update_t)(rmqbit* q, uint a, uint n);

void longpat(uchar* s, uint n, uint* r, uint* h, uint* p, bittree* bwtt,
	longpat_callback fun, void* data) {
	uint i, n1 = n+1;
	uint x, a, b, dl;
	uint v, va, vb, vu, vl;
	rmq *rh;
	rmqbit *rrv;
	uint *rv;
	uint use_rmqbit = 1;
	rmqbit_malloc_t* f_rmqbit_malloc = use_rmqbit?rmqbit_malloc:rmq_malloc;
	rmqbit_free_t* f_rmqbit_free = use_rmqbit?rmqbit_free:rmq_free;
	rmqbit_init_t* f_rmqbit_init_max = use_rmqbit?rmqbit_init_max:rmq_init_max;
	rmqbit_query_t* f_rmqbit_query_max = use_rmqbit?rmqbit_query_max:rmq_query_max;
	rmqbit_update_t* f_rmqbit_update_max = use_rmqbit?rmqbit_update_max:rmq_update_max;

	rh = rmq_malloc(n); // 0.125 * N
	h[n-1] = 0;
	rmq_init_min(rh, h, n);

	rrv = f_rmqbit_malloc(n);
	rv = (uint*)pz_malloc(n * sizeof(uint));
	memset(rv, 0, n * sizeof(uint));
	f_rmqbit_init_max(rrv, rv, n);

	forn(i, n) {
		x = p[i];

		a = bittree_max_less_than(bwtt, n1, x); // must check if a==0. If x==0, we cannot extend x to the left, so a=0 is ok.
		b = bittree_min_greater_than(bwtt, n1, x+1); // must check if b==n
		// The range of rows [a..b) is a /block/ (of at least 1 row (x)).

		va = 0;
		if (a != 0) { // There exists a row up, with different last char
			va = rmq_query_min(rh, a-1, x); // LCP of rows [a-1..x]
		}

		vb = 0;
		if (b != n) { // There exists a row down, with different last char
			vb = rmq_query_min(rh, x, b); // LCP of rows [x..b]
		}
		v = (va<vb)?vb:va;

//		uint j; fprintf(stderr, "i=%2u, x=%2u, a=%2u, b=%2u, v=%2u ", i, x, a, b, v); forn(j, v) fprintf(stderr, "%c", s[i+j]); fprintf(stderr, "\n");
		do {
			// PAT_LONG
			dl = 0;

			if (v > 0) {
				vl = x?rmq_preveq_leq(rh, x-1, v-1, n):n; if (vl == n) vl = 0; else vl++;
				vu = rmq_nexteq_leq(rh, x, v-1, n); if (vu < n) vu++;
				// [vl, vu) is a block of (vu-vl) rows, of width v, maximal in v to the left and to the right (aka a pattern).
				// PAT_DELTA
				dl = f_rmqbit_query_max(rrv, vl, vu);

//				fprintf(stderr, "****, va=%2u, vb=%2u, v=%2u, vl=%2u, vu=%2u, dl=%2u \n", va, vb, v, vl, vu, dl);
				if (dl == 0) {
					// There's no previous occurence, so iterate again
					va = vl?h[vl-1]:0;
					vb = (vu<n)?h[vu-1]:0;
					v = (va<vb)?vb:va;
				} else {
					dl = i - (dl-1); // OK =)
				}
			}
		} while (v && !dl);
		rv[x] = i+1;
		f_rmqbit_update_max(rrv, x, n);

		// PAT_LONG = v
		// PAT_DELTA = dl
//		fprintf(stderr, ">>    dl=%2u, v=%2u \n", dl, v);
		fun(i, v, dl, data);
	}

	f_rmqbit_free(rrv, n);
	rmq_free(rh, n);
	pz_free(rv);

}
