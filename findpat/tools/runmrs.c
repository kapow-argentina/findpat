#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "lcp.h"
#include "mrs.h"
#include "mrs_nolcp.h"
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
	uint n,i;
	uint ml = 1;
	bool opt_nolcp = FALSE;
	int xargc = 0;
	char *xargv[1] = { NULL };
	forsn(i, 1, argc) {
		if (0);
		else cmdline_opt_1(i, "--nolcp") { opt_nolcp = TRUE; }
		else cmdline_opt_2(i, "--ml") { ml = atoi(argv[i]); }
		else {
			if (xargc == 1) {
				fprintf(stderr, "%s: Too many arguments: %s\n", argv[0], argv[i]);
				return 1;
			}
			xargv[xargc++] = argv[i];
		}
	}
	if (xargc < 1) {
		fprintf(stderr, "Usage: %s <string> [<options>]\n", argv[0]);
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "  --ml <ml>\n");
		fprintf(stderr, "  --nolcp\n");
		return 1;
	}

	s = (uchar*)xargv[0];
	n = strlen(xargv[0]) + 1;
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
	if (opt_nolcp) {
		mrs_nolcp(s, n, r, h, p, ml, output_readable, &ord);
	} else {
		mrs(s, n, r, h, p, ml, output_readable, &ord);
	}
	pz_free(h);
	pz_free(r);
	pz_free(p);
	return 0;
}
