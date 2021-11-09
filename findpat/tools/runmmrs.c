#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "lcp.h"
#include "mmrs.h"
#include "tipos.h"
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

int main(int argc, char** argv) {
	uint *p, *r, *h;
	uchar *s;
	uint n,i,ml;
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <string> [ml]\n", argv[0]);
		return 1;
	}
	if (argc == 3) ml = atoi(argv[2]); else ml = 1;

	s = (uchar*)argv[1];
	n = strlen(argv[1]) + 1;
	s[n-1] = 255;
	p = (uint*)pz_malloc(n*sizeof(uint));
	r = (uint*)pz_malloc(n*sizeof(uint));
	h = (uint*)pz_malloc(n*sizeof(uint));
	bwt(NULL, p, r, s, n, NULL);
	forn(i,n) p[r[i]]=i;
	memcpy(h, r, n*sizeof(uint));
	lcp(n, s, h, p);

//	show_bwt_lcp(n, s, r, h);

	output_readable_data ord;
	ord.r = r;
	ord.s = s;
	ord.fp = stdout;
	mmrs(s, n, r, h, ml, output_readable, &ord);
	pz_free(h);
	pz_free(r);
	pz_free(p);
	return 0;
}
