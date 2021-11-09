#include "macros.h"
#include "sa_search.h"

#define midpoint(_L, _U)	(((_L) >> 1) + ((_U) >> 1) + (1 & (_L) & (_U)))
#define IMP(x, y) (!(x) || (y))
bool sa_search(uchar *s, uint *r, uint n, uchar *pat, uint len, uint *from, uint *to) {
			/*output_callback out, void* out_data) {*/

/* Update L, U to L', U' so that L' == U' + 1, L <= L', U' <= U and
 *   U' is the leftmost index s.t. s[RI(U')] not[OP] VAL if there is any.
 *   U' == U  if not
 */
#define binsearch(L, U, RI, VAL, OP) { \
	while (L + 1 < U) { \
		uint _mid = midpoint(L, U); \
		if (RI(_mid) < n && s[RI(_mid)] OP VAL) L = _mid; \
		else U = _mid; \
	} \
}
	uint lower, upper, i;
	uint nu, nl, val;
	uint lcp = 0;

	lower = 0;
	upper = n;
	for (i = lcp; lower < upper && i < len; ++i) {
#define _ri_izq(x) (r[(x) - 1] + (i))
#define _ri_der(x) (r[(x)] + (i))
		/* Find the new boundaries */
		val = pat[i];
		if (s[r[lower] + i] > val) {
			upper = lower;
		} else if (s[r[upper - 1] + i] < val) {
			lower = upper;
		} else {
			nu = upper, nl = lower;
			binsearch(lower, nu, _ri_izq, val, <);
			binsearch(nl, upper, _ri_der, val, <=);
		}
#undef _ri_izq
#undef _ri_der
#undef binsearch
	}

	*from = lower;
	*to = upper;
	return lower < upper;
}

/* Manber and Myers' algorithm */

/* Macros for sa_mm_search */

#define _update_lcp(ri, h, st, SRC) for(h = st; h < len && (ri)+h < n && SRC((ri)+h) == pat[h]; h++)
#define _binary_step(low, mid, upp, hlow, hmid, hupp, SRC) { \
		mid = midpoint(low, upp); \
		rmid = r[mid]; \
		if (hlow >= hupp) { \
			if (llcp[mid] >= hlow) _update_lcp(rmid, hmid, hlow, SRC); \
			else hmid = llcp[mid]; \
		} else { \
			if (rlcp[mid] >= hupp) _update_lcp(rmid, hmid, hupp, SRC); \
			else hmid = rlcp[mid]; \
		} }


#define sa_mm_search_fun(ST, ED, SRC) { \
	uint lower=ST, upper=ED; /* Result boundaries */ \
	uint hlow=0, hmid, hupp=0; \
	uint low, mid, upp; /* Binary search boundaries */ \
\
	uint b_low=ST, b_hlow, b_upp=ED-1, b_hupp; \
	uint rmid; \
\
	const uint r0 = r[ST], rn1 = r[ED-1]; \
	_update_lcp(r0,  hlow, 0, SRC); /* hlow = LCP(pat, s[r[0]..n]) */ \
	_update_lcp(rn1, hupp, 0, SRC); /* hupp = LCP(pat, s[r[n-1]..n]) */ \
	b_hlow = hlow; b_low = ST; \
	b_hupp = hupp; b_upp = ED-1; \
\
	uint b_iter = b_hupp < len; \
	if (hlow < len) { \
		low=ST; upp=ED-1; \
		while (upp > low+1) { \
			_binary_step(low, mid, upp, hlow, hmid, hupp, SRC) \
			/* fprintf(stderr, "L low,mid,upp = (%3d,%3d,%3d), hlow,hmid,hupp = (%3d,%3d,%3d)\n", low, mid, upp, hlow, hmid, hupp); */ \
			if (b_iter == 1 && hmid == len) { \
				/* Each side chooses a different path */ \
				b_hlow = hmid; b_low = mid; \
				b_hupp = hupp; b_upp = upp; \
				b_iter = 2; \
			} \
			if (hmid == len || (rmid+hmid < n && pat[hmid] <= SRC(rmid+hmid))) { \
				hupp = hmid; upp = mid; \
			} else { \
				hlow = hmid; low = mid; \
			} \
		} \
		if (upp == ED-1 && hupp < len && (rn1+hupp >= n || pat[hupp] > SRC(rn1+hupp))) upp++; \
		if (low == ST && hlow < len && r0+hlow < n && pat[hlow] < SRC(r0+hlow)) upp=ST; \
		lower = upp; \
		if (b_iter == 1) upper = upp; /* String not found */ \
	} else { \
		if (hupp < len) { \
			b_iter = 2; \
			b_low = ST; b_upp = ED-1; \
		} \
	} \
\
	if (b_iter == 2) { \
		upp = b_upp; low = b_low; \
		hupp = b_hupp; hlow = b_hlow; \
		while (upp > low+1) { \
			_binary_step(low, mid, upp, hlow, hmid, hupp, SRC) \
			/* fprintf(stderr, "U low,mid,upp = (%3d,%3d,%3d), hlow,hmid,hupp = (%3d,%3d,%3d)\n", low, mid, upp, hlow, hmid, hupp); */ \
			if (hmid == len || pat[hmid] >= SRC(rmid+hmid)) { \
				hlow = hmid; low = mid; \
			} else { \
				hupp = hmid; upp = mid; \
			} \
		} \
		upper = upp; \
	} \
	\
	*from = lower; \
	*to = upper; \
	return (lower < upper); \
}

#define _SRC_s(X) s[X]
bool sa_mm_search_8(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const uchar* llcp, const uchar* rlcp)
	{ sa_mm_search_fun(0, n, _SRC_s) }

bool sa_mm_search_16(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const ushort* llcp, const ushort* rlcp)
	{ sa_mm_search_fun(0, n, _SRC_s) }

bool sa_mm_search_32(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const uint* llcp, const uint* rlcp)
	{ sa_mm_search_fun(0, n, _SRC_s) }
#undef _SRC_s

#define _SRC_dna(X) ("ACGT"[(s[(X)/4] >> 2*((X)%4)) & 3])
bool sa_mm_search_dna_8(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const uchar* llcp, const uchar* rlcp, uint lookup, uint* lk) {
	uint st=0, ed=n;
	if (lookup) {
		uint lka=0, lkb=0, i;
		for(i=0; i<len && i<lookup; i++) {
			lka <<= 2;
			switch (pat[i]) {
				case 'A': lka |= 0; break;
				case 'C': lka |= 1; break;
				case 'G': lka |= 2; break;
				case 'T': lka |= 3; break;
			}
		}
		lkb = lka-1;
		if (i<lookup) { i = 2*(lookup-i); lka = ((lka+1)<<i)-1; lkb = ((lkb+1)<<i)-1; }
		/* Get the range [lkb..lka) */
		if (lkb != -1) st = lk[lkb];
		ed = lk[lka];
		// fprintf(stderr, "lk[%u,%u] --> [%u, %u)\n", lkb, lka, st, ed);
		if (st == ed) { *from = *to = st; return FALSE; }
		if (ed-st == 1) {
			uint rst = r[st]; uint hres = 0;
			_update_lcp(rst, hres, 0, _SRC_dna);
			if (hres == len) { *from = st; *to = ed; return TRUE; }
			if (rst+hres < n && _SRC_dna(rst+hres) > pat[hres]) ed--;
			*from = *to = ed; return FALSE;
		}
	}
	sa_mm_search_fun(st, ed, _SRC_dna)
}

/* Binary search initialization */

static __thread void* _h;
static __thread void* _llcp;
static __thread void* _rlcp;

#define _mm_build_lrlcp(funcname, uinttype, TOPE) \
static uint lrlcp_rec##funcname(uint lm, uint rm) { \
	if (lm >= rm) return (uint)-1; \
	else if (rm == lm + 1) { \
		return ((uinttype*)_h)[lm]; \
	} else /* (rm - lm > 1) */ { \
		uint m = midpoint(lm,rm); \
		const uint lv = lrlcp_rec##funcname(lm, m); \
		const uint rv = lrlcp_rec##funcname(m, rm); \
		((uinttype*)_llcp)[m] = lv>TOPE?TOPE:lv; \
		((uinttype*)_rlcp)[m] = rv>TOPE?TOPE:rv; \
		return (lv<rv)?lv:rv; \
	} \
} \
\
void build_lrlcp_lk##funcname(uint n, uinttype* h, uinttype* llcp, uinttype* rlcp, uint lookup, uint* lk) { \
	_llcp = llcp; \
	_rlcp = rlcp; \
	_h = h; \
	if (lookup && lk) { \
		uint i = 0, lv=0, nlkp = 1UL << (lookup*2); \
		forn(i, nlkp) if (lk[i] != lv) { lrlcp_rec##funcname(lv, lk[i]-1); llcp[lv] = rlcp[lv] = 0; lv = lk[i]; llcp[lv-1] = rlcp[lv-1] = 0; } \
	} else lrlcp_rec##funcname(0, n-1); \
	/* Set the 'unused' values to something (should I do this here? or in the upper function) */ \
	llcp[0] = rlcp[0] = 0; \
	llcp[n-1] = rlcp[n-1] = 0; \
	_h = _llcp = _rlcp = NULL; /* Clean the pointers */ \
} \
\
void build_lrlcp##funcname(uint n, uinttype* h, uinttype* llcp, uinttype* rlcp) { \
	build_lrlcp_lk##funcname(n, h, llcp, rlcp, 0, NULL); \
}

_mm_build_lrlcp(_8, uchar, 0xFFU)
_mm_build_lrlcp(_16, ushort, 0xFFFFU)
_mm_build_lrlcp(_32, uint, 0xFFFFFFFFU)
