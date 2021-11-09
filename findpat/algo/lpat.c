#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <assert.h>

#include "macros.h"
#include "lcp.h"
#include "cop.h"

#define EQUAL(x, y)	((x) != '~' && (x) == (y))

void lcp_ignore_Ns(uint n, uchar* s, uint* r, uint* p) {
	uint h = 0, i, j;
	forn(i,n) if (p[i] > 0) {
		j = r[p[i]-1];
		while(h < n && EQUAL(s[(i+h)%n], s[(j+h)%n])) ++h;
		r[p[i]-1] = h;
		if (h > 0) --h;
	}
}

void extend_lcp_to_patterns(uint n, uint *hp, uint *stack);

void longest_mrs(uint n, uchar *s, uint *r, uint *p) {
#define h r
#define hp p
#define stack r
	uint i;

	/* h[i] := LCP(s[i..], s1) */
	lcp_ignore_Ns(n, s, h, p);

	/* h[i] := max(LCP(s[i..],s1), LCP(s[i..],s2)) */
	uint ho = 0, tmp;
	forn (i, n - 1) {
		tmp = h[i];
		if (ho > h[i]) h[i] = ho;
		ho = tmp;
	}
	h[n - 1] = ho;

	/* hp := permutation of h in the order of the string */
	forn (i, n) hp[i] = h[p[i]];

	extend_lcp_to_patterns(n, hp, stack);
#undef h
#undef hp
#undef stack
}

void longest_mrs_full2(uint n, uint long_input_a, uchar *s, uint *r, uint *p) {
	uint *h = pz_malloc(n * sizeof(uint));
	memcpy(h, r, n * sizeof(uint));
	lcp_ignore_Ns(n, s, h, p);
	uint *m = p;

	mcl(r, h, n, m, long_input_a);
	mcl_reverse(r, h, n, &m[long_input_a], n - long_input_a);
	extend_lcp_to_patterns(n, m, r);
	pz_free(h);
}

/*
 * Input:
 *   n     : size of the input
 *   hp[i] : max "l" such that s[i..(i + l)] satisfies a property P
 *           (for a certain string "s")
 *   stack : temporary array (of n positions)
 *
 * Output:
 *   hp[i] : max "l" such that there's a substring of s
 *           of length l that satisfies the property P and
 *           overlaps "i"
 *
 * P should be such that: P(s[i..j]) ==> P(s[i+1..j])
 * Typically: P(x) = x is a substring of t
 *
 */
void extend_lcp_to_patterns(uint n, uint *hp, uint *stack) {
	uint i;
	bzero(stack, sizeof(uint) * n);
	uint k = 0; /* top of the stack */
	forn (i, n) {
		const uint hval = hp[i];
		stack[hval] = i;
		if (hval > k) k = hval;
		while (k > 0 && i >= stack[k] + k) k--;
		hp[i] = k; /* save the result */
	}
}

