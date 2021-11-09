#include "rmq.h"
#include <stdlib.h>

#include "macros.h"

rmq* rmq_malloc(uint n) {
	uint i, m, logn = 1, sz = 0;
	rmq* res;
	uint* buf;
	for(i=1;i<n;i<<=1) ++logn;
	res = (rmq*)pz_malloc(logn * sizeof(uint*));

	res[0] = NULL;
	if (logn > 1) {
		m = n;
		forsn(i, 1, logn) sz += (m = (m>>1)+(m&1));
		buf = (uint*)pz_malloc(sz * sizeof(int));

		forsn(i, 1, logn) {
			sz -= (n = (n>>1)+(n&1));
			res[i] = buf+sz;
		}
	}
	return res;
}

void rmq_free(rmq* q, uint n) {
	uint i, logn = 1;
	for(i=1;i<n;i<<=1) ++logn;
	if (logn > 1) pz_free(q[logn-1]);
	pz_free(q);
}

#define _def_rmq(op, one) \
void rmq_init##op(rmq* q, uint* vec, uint n) { uint i, l = 0; q[0] = vec; while (n>1) { const uint lm = n; n = (n>>1)+(n&1); l++; forn(i, n-(lm&1)) q[l][i] = op(q[l-1][2*i], q[l-1][2*i+1]); if (lm&1) q[l][n-1] = q[l-1][2*n-2]; } } \
uint rmq_query##op(const rmq* const q, uint a, uint b) { uint l = 0, res = q[0][a]; while(a < b) { if (a&1) { res = op(res, q[l][a]); a++;} if (b&1) { b--; res = op(res, q[l][b]);} a/=2; b/=2; l++; } return res; } \
void rmq_update##op(rmq* q, uint a, uint n) { uint l = 0, vl; while ((n>1) && (vl = ((a^1)<n)?op(q[l][a], q[l][a^1]):q[l][a]) != q[l+1][a/2]) { n = (n>>1)+(n&1); q[++l][a/=2] = vl; } } \
uint rmq_nexteq##one(rmq* q, uint i, uint vl, uint n) { uint l = 0, m = n; \
	while ((i < m) && !(one(q[l][i], vl))) { if (i&1) { i++; } else { i /= 2; ++l; m = (m>>1)+(m&1); } } \
	if (i >= m) return n; /* FAIL */ while (l>0) { i *= 2; --l; if (!one(q[l][i], vl)) i++; } return i; } \
uint rmq_preveq##one(rmq* q, uint i, uint vl, uint n) { uint l = 0, m = n; \
	while ((i < m) && !(one(q[l][i], vl))) { if (!(i&1)) { i--; } else { i /= 2; ++l; m = (m>>1)+(m&1); } } \
	if (i >= m) return n; /* FAIL */ while (l>0) { i *= 2; --l; if (one(q[l][i+1], vl)) i++; } return i; }

#define _leq(a, b) ((a)<=(b))
#define _min(a, b) (_leq(a, b)?(a):(b))

#define _geq(a, b) ((a)>=(b))
#define _max(a, b) (_geq(a, b)?(a):(b))

_def_rmq(_min, _leq)
_def_rmq(_max, _geq)
