#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "lcp.h"
#include "longpat.h"
#include "bittree.h"
#include "tipos.h"
#include "common.h"

#include "macros.h"

void show_bwt_lcp(uint n, uchar* src, uint* r, uint* h) {
	int i, j;
	printf("\n");
	printf(" i  r[] lcp \n");
	forn(i, n) {
		printf("%3d %3d %3d ", i, r[i], h[i]);
		forn(j, n) printf("%c", src[(r[i]+j)%n]);
		printf("\n");
	}
}

typedef struct str_data_greedy {
	uint cost;
	uint lim;
	uchar* s;
} data_greedy;
static void dg_init(data_greedy* dg) {
	dg->cost = 0;
	dg->lim = 0;
}

static void greedy_step(uint i, uint v, uint dl, data_greedy* dg) {
	uint nlim;
	uint ncost;
	const uint chcost = 8;
	if (v != 0) {
		nlim = i + v;
		ncost = bneed(v) + bneed(dl);
		if (ncost < (nlim - dg->lim) * chcost) {
			fprintf(stderr, "at %u, (%u, %u)\n", i, v, dl);
			dg->lim = nlim;
			dg->cost += ncost;
		}
	}
	if (i >= dg->lim) {
		fprintf(stderr, "at %u, char %c\n", i, dg->s[i]);
		dg->lim = i+1;
		dg->cost += chcost;
	}
}


int main(int argc, char** argv) {
	uint *p, *r, *h;
	uchar *s, *bwto;
	bittree *bwtt;
	uint n;
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <string>\n", argv[0]);
		return 1;
	}

	s = (uchar*)argv[1]; // 1*N
	n = strlen(argv[1]) + 1;
	s[n-1] = 255;
	p = (uint*)pz_malloc(n*sizeof(uint)); // 4*N
	r = (uint*)pz_malloc(n*sizeof(uint)); // 4*N
	bwto = (uchar*)pz_malloc(n*sizeof(uchar)); // 1*N
	bwt(bwto, p, r, s, n, NULL);

	bwtt = longpat_bwttree(bwto, n); // 0,25 *N
	pz_free(bwto); // -1 *N

	h = (uint*)pz_malloc(n*sizeof(uint)); // 4*N

	// Como ponemos la salida de bwt (bwto) en otro lado, p queda intacto.
//	forn(i,n) p[r[i]]=i;

	memcpy(h, r, n*sizeof(uint));
	lcp(n, s, h, p);
	show_bwt_lcp(n, s, r, h);

	data_greedy dg;
	dg_init(&dg);
	dg.s = s;

	longpat(s, n, r, h, p, bwtt, (longpat_callback*)greedy_step, &dg);

	bittree_free(bwtt, n+1);
	DBGu(dg.lim);
	DBGu(dg.cost);
	DBGu((dg.cost+7)/8);

	pz_free(h);
	pz_free(r);
	pz_free(p);
	return 0;
}

