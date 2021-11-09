#include "mrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

#include "bittree.h"
#include "sorters.h"

#include "macros.h"
#include "output_callbacks.h"

/*
 * Convert an array A of n elements, which are a multiset
 * to another array B of n elements, which are the set {0, ..., n - 1}
 * such that (A[i] <= A[j])  ==>  (B[i] <= B[j])
 *
 * It is stable, i.e.
 *     (A[i] == A[j] && i < j)  ==>  (B[i] < B[j])
 *
 * Time:   O(n)
 * Memory: sizeof(uint) * max(A)
 */
#define USE_AUX_ARRAY 1		/* Use memory from aux_array instead of mallocing */

void _scale(uint *a, uint n, uint *aux_array) {
	uint i;
	/* Find max */
	uint mx = 0;
	forn (i, n) if (a[i] > mx) mx = a[i];
	++mx;
	/* Count in buckets */
#ifdef USE_AUX_ARRAY
	uint *bc = aux_array;
#else
	uint *bc = pz_malloc(mx * sizeof(uint));
#endif
	bzero(bc, mx * sizeof(uint));
	forn (i, n) bc[a[i]]++;
	/* Accumulate buckets */
	uint acc = 0;
	forn (i, mx) {
		const uint tmp = bc[i];
		bc[i] = acc;
		acc += tmp;
	}
	/* Scale */
	forn (i, n) a[i] = bc[a[i]]++;
#ifndef USE_AUX_ARRAY
	pz_free(bc);
#endif
}

/*
 * Convert an array A with elements, which are a permutation of the set {0, ..., n - 1},
 * to another array B of n elements, which is the inverse permutation of A.
 *
 * I.e. A[B[i]] == B[A[i]] == i  forall 0 <= i < n
 *
 * Time:   O(n)
 * Memory: bitarray of n bits
 */

#define USE_EXTRA_BITARRAY 1

#if USE_EXTRA_BITARRAY
/* Use an extra bitarray, allowing values up to 2^32 */

#if USE_AUX_ARRAY /* Use memory from aux_array instead of mallocing */
#define initialize(N) \
	bitarray *_ba = aux_array; \
	bita_clear(_ba, (N));
#define cleanup() 		/* nop */
#else /* !USE_AUX_ARRAY */
#define initialize(N) \
	bitarray *_ba = bita_malloc((N)); \
	bita_clear(_ba, (N));
#define cleanup() 		pz_free(_ba)
#endif /* USE_AUX_ARRAY */
#define flip(A, I, X) 		{ ((A)[(I)] = (X)); bita_set(_ba, (I)); }
#define unflip(A, I) 		/* nop */
#define is_done(A, I) 		bita_get(_ba, (I))

#else /* !USE_EXTRA_BITARRAY */

/* Do not use an extra bitarray, but allow values up to 2^31 */
#define initialize(n) 		/* nop */
#define cleanup() 		/* nop */
#define flip(A, I, X) 		((A)[(I)] = (~(X)))
#define unflip(A, I) 		flip((A), (I), (A)[(I)])
#define is_done(A, I) 		(((A)[(I)]) & 0x80000000)

#endif /* USE_EXTRA_BITARRAY */

void _invert_permutation(uint *a, uint n, uint *aux_array) {
	uint i = 0;
	uint elem = a[0];
	initialize(n);
	while (i < n) {
		const uint next_elem = a[elem];
		flip(a, elem, i);
		i = elem;
		elem = next_elem;
		if (is_done(a, elem)) {
			for (; i < n && is_done(a, i); ++i) {
				unflip(a, i);
			}
			if (i < n) elem = a[i];
		}
	}
	cleanup();
}
#undef cleanup
#undef unflip
#undef flip
#undef is_done
#undef initialize

#define assert_ assert
#define IMP(p, q) 	(!(p) || (q))
void mrs_nolcp(uchar* s, uint n, uint* r, uint* h, uint* p, uint ml,
		 output_callback out, void* data) {
	uint i,ii,j,k,n1=n+1;
	bittree* tree = bittree_malloc(n1);
	bittree_clear(tree, n1);
	bittree_preset(tree,n1,0);
	bittree_preset(tree,n1,n);
	forn(i,n-1) if (h[i] < ml) bittree_preset(tree,n1,i+1);
	bittree_endset(tree,n1);

	/* Transform h into its own sorting permutation */
	/* Use p as an auxiliary array */
	_scale(h, n - 1, p);
	_invert_permutation(h, n - 1, p);
//	fprintf(stderr, "mrs: sort end\n");

	forn(i,n) p[r[i]] = i;

	uint h_i = 0;		/* value of the current LCP */
	uint last_hi = 0;	/* last position which held the current LCP */

	forn (ii, n - 1) {
		i = h[ii];
		{ /* Update h_i to match the current LCP */
			uchar *s1 = &s[r[i] + h_i];
			uchar *s2 = &s[r[i+1] + h_i];
			if (*s1 == *s2) last_hi = i;
			while (*s1 == *s2) ++h_i, ++s1, ++s2;
		}
		if (h_i < ml) continue;

		j = bittree_max_less_than(tree, n1, i+1);

		if (j > 0 && j - 1 == last_hi) {
			last_hi = i;
			bittree_set(tree, n1, i+1);
			continue;
		}
		k = bittree_min_greater_than(tree, n1, i+1) - 1;
		last_hi = i;
		bittree_set(tree, n1, i+1);

		if (r[j] > 0 && r[k] > 0 && s[r[j]-1] == s[r[k]-1]
			&& p[r[k]-1]-p[r[j]-1]==k-j) continue;
		out(h_i, j , k-j+1, data);
	}
	bittree_free(tree, n1);
}

