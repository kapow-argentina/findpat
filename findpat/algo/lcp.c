#include "lcp.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "macros.h"

#ifdef _ENABLE_LCP_SEPARATOR
#define EQUAL(x, y)	((x) != _LCP_SEPARATOR && (x) == (y))
#else
#define EQUAL(x, y)	((x) == (y))
#endif

#define __lcp_func(EQ, h) { \
	uint hv = 0, i, j; \
	forn(i,n) if (p[i] > 0) { \
		j = r[p[i]-1]; \
		while(hv < n && EQ(s[(i+hv)%n], s[(j+hv)%n])) ++hv; \
		h[p[i]-1] = hv; \
		if (hv > 0) --hv; \
	} \
}

/* This is the same function as __lcp_func but uses the temporary
 * location "p" with  n/BLOCKS uints.
 * As a drawback, it cost BLOCKS more time */
#define __lcp_mem_func(EQ, BLOCKS, TOPE, h) { \
	uint hv, m = (n+BLOCKS-1)/BLOCKS, *p, *_p, a, b, i, j; \
	_p = pz_malloc(sizeof(uint)*m); if (!_p) return; \
	for(a=0; a<n; a+=m) { \
		b = (a+m)<n?a+m:n; p = _p-a; hv = 0; \
		forn(i,n) if (r[i] >= a && r[i] < b) p[r[i]] = i; \
		forsn(i,a,b) if (p[i] > 0) { \
			j = r[p[i]-1]; \
			while(i+hv < n && j+hv < n && hv < TOPE && EQ(s[i+hv], s[j+hv])) ++hv; \
			h[p[i]-1] = hv; \
			if (hv > 0) --hv; \
		} \
	} \
	pz_free(_p); \
}

void lcp(uint n, const uchar* s, uint* r, uint* p)
	__lcp_func(EQUAL, r)

#define EQ_EQ(x, y)	((x) == (y))
void lcp3(uint n, const uchar* s, const uint* r, uint* p, uint* h)
	__lcp_func(EQ_EQ, h)

#define EQ_SP(x, y)	((x) != sp && (x) == (y))
void lcp3_sp(uint n, const uchar* s, const uint* r, uint* p, uint* h, unsigned char sp)
	__lcp_func(EQ_SP, h)

#define EQ_EQ(x, y)	((x) == (y))
void lcp_mem3(uint n, const uchar* s, const uint* r, uchar* h)
	__lcp_mem_func(EQ_EQ, 4, 255, h)

#define EQ_SP(x, y)	((x) != sp && (x) == (y))
void lcp_mem3_sp(uint n, const uchar* s, const uint* r, uchar* h, unsigned char sp)
	__lcp_mem_func(EQ_SP, 4, 255, h)
