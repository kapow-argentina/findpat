#include "rmqbit.h"
#include <stdlib.h>
#include "bitarray.h"

#include "macros.h"

#define roundup(x, m) (((x)+((m)-1))&~(m-1))

rmqbit* rmqbit_malloc(uint n) {
	uint i, m, logn = 1, sz = 0;
	rmqbit* res;
	bitarray* buf;
	for(i=1;i<n;i<<=1) ++logn;
	res = (rmqbit*)pz_malloc(logn * sizeof(uint*));

	res[0] = NULL;
	if (logn > 1) {
		m = n;
		forsn(i, 1, logn) sz += roundup((m = (m>>1)+(m&1)), ba_word_size);
		buf = bita_malloc(sz);

		m = n;
		forsn(i, 1, logn) {
			sz -= roundup((m = (m>>1)+(m&1)), ba_word_size);
			res[i] = buf+sz/ba_word_size;
		}
	}
	return res;
}

void rmqbit_free(rmqbit* q, uint n) {
	uint i, logn = 1;
	for(i=1;i<n;i<<=1) ++logn;
	if (logn > 1) pz_free(q[logn-1]);
	pz_free(q);
}

uint rmqbit_val(const rmqbit* const q, uint a, uint l) { for(;l;l--) a = bita_get(q[l], a) | (2*a); return q[0][a]; }

#define _def_rmq(op) \
void rmqbit_init##op(rmqbit* q, uint* vec, uint n) { uint i, l = 0; q[0] = vec; \
	while (n>1) { \
		const uint lm = n; n = (n>>1)+(n&1); l++; \
		forn(i, n-(lm&1)) { \
			uint x = rmqbit_val(q, 2*i, l-1), y = rmqbit_val(q, 2*i+1, l-1); \
			if (op(x, y)==x) bita_unset(q[l],i); else bita_set(q[l],i); \
		} \
		if (lm&1) bita_unset(q[l], n-1); } } \
uint rmqbit_query##op(const rmqbit* const q, uint a, uint b) { \
	uint l = 0, w, res = rmqbit_val(q, a, 0); while(a < b) { \
		if (a&1) { w = rmqbit_val(q, a, l); res = op(res, w); a++; } \
		if (b&1) { b--; w = rmqbit_val(q, b, l); res = op(res, w); } \
		a/=2; b/=2; l++; \
	} return res; } \
void rmqbit_update##op(rmqbit* q, uint a, uint n) { uint l = 0, vl=q[0][a], x, y, c;  for (;n>1; n=(n>>1)+(n&1)) { c = a&1; \
	x = vl; if ((a^1)<n) { y = rmqbit_val(q, a^1, l); vl = op(x,y); if ((c^bita_get(q[l+1], a/2)) && (vl==y)) break; } \
	l++; a/=2; if ((vl==x)^c) bita_unset(q[l], a); else bita_set(q[l], a); } }

#ifdef _DEBUG
void rmqbit_show(rmqbit* q, uint n) {
	uint l, i, j, sp=1, sz = 0;
	forn(i, n) fprintf(stderr, "%4u ", q[0][i]); fprintf(stderr, "\n");
	for (l=1;n>1; l++,(sp = sp*2+1),n=(n>>1)+(n&1)) {
		int m = (n>>1)+(n&1); sz += roundup(m, ba_word_size);
		forn(i, sp*3-1) fprintf(stderr, " ");
		forn(i, m) {
			fprintf(stderr, "%4u,%c", rmqbit_val(q, i, l), "<>"[bita_get(q[l], i)]);
			forn(j, sp*5-1) fprintf(stderr, " ");
		}
		fprintf(stderr, "\n");
	}
}
void rmqbit_bitshow(rmqbit* q, uint n) {
	uint l, i, j, sp=1;
	fprintf(stderr, "l=0: "); forn(i, n) fprintf(stderr, "* "); fprintf(stderr, "\n");
	for (l=1;n>1; l++,(sp = sp*2+1),n=(n>>1)+(n&1)) {
		fprintf(stderr, "l=%u: ", l);
		int m = (n>>1)+(n&1);
		forn(i, sp) fprintf(stderr, " ");
		forn(i, m) {
			fprintf(stderr, "%c", "<>"[bita_get(q[l], i)]);
			forn(j, sp*2+1) fprintf(stderr, " ");
		}
		fprintf(stderr, "\n");
	}
}
#endif

#define _min(a, b) ((a)<(b)?(a):(b))
#define _max(a, b) ((a)>(b)?(a):(b))

_def_rmq(_min)
_def_rmq(_max)
