#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "lcp.h"
#include "klcp.h"
#include "tipos.h"
#include "macros.h"

int main(int argc, char** argv) {
	uint *p, *r, *h;
	uchar *s = (uchar*)argv[1];
	uint n,i,ii,j,is = 0;
	bool opt_bare = FALSE, opt_orig = FALSE;
	if (argc < 2 || argc > 4) {
		fprintf(stderr, "Usage: %s <string> [<options>]\n", argv[0]);
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "  -b     print only LCP values\n");
		fprintf(stderr, "  -o     print in the original order\n");
		return 1;
	}
	forsn(i, 1, argc) {
		if (0);
		else cmdline_var(i, "b", opt_bare)
		else cmdline_var(i, "o", opt_orig)
		else {
			if (is) {
				fprintf(stderr, "Usage: %s <string> [<options>]\n", argv[0]);
				fprintf(stderr, "Options:\n");
				fprintf(stderr, "  -b     print only LCP values\n");
				fprintf(stderr, "  -o     print in the original order\n");
				return 1;
			} else is = i;
		}
	}
	n = strlen(argv[is]);
	argv[is][n] = (char)255;
	p = (uint*)pz_malloc((n+1)*sizeof(uint));
	r = (uint*)pz_malloc((n+1)*sizeof(uint));
	h = (uint*)pz_malloc((n+1)*sizeof(uint));
	memcpy(p, argv[is], n+1);
	bwt(NULL, p, r, NULL, n+1, NULL);
	forn(i,n+1) p[r[i]]=i;
	memcpy(h, r, (n+1)*sizeof(uint));
	lcp(n+1, s, h, p);
	h[n] = 0;

	if (!opt_bare) {
		printf("LCP Pos Pfx\n");
		forn(ii,n) {
			i = opt_orig ? ii : r[ii];
			printf("%3d %3d %3d ", h[p[i]], p[i], i);
			forsn(j,i,n) printf("%c",argv[is][j]); printf("\n");
		}
		double kval = klcp(n, h);
		printf("klcp = %.3lf\n", kval);
	} else {
		if (opt_orig) forn(i,n-1) printf("%u\n", h[p[i]]);
		else forn(i,n-1) printf("%u\n", h[i]);
	}
	
	pz_free(r);
	pz_free(h);
	pz_free(p);
	return 0;
}
