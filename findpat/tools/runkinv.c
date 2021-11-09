#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "lcp.h"
#include "klcp.h"
#include "tipos.h"
#include "macros.h"

/* Nota:
 * El string de entrada se considera con un terminador ($) como el caracter 0.
 * El valor de n en consecuencia es strlen(argv[1])+1.
 */

int main(int argc, char** argv) {
	uint *p, *r, *h;
	uchar *s = (uchar*)argv[1];
	uint n,i;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <string>\n", argv[0]);
		return 1;
	}

	n = strlen(argv[1]);
	p = (uint*)pz_malloc((n+1)*sizeof(uint));
	r = (uint*)pz_malloc((n+1)*sizeof(uint));
	h = (uint*)pz_malloc((n+1)*sizeof(uint));
	memcpy(p, argv[1], n+1);
	bwt(NULL, p, r, NULL, n+1, NULL);
	forn(i,n+1) p[r[i]]=i;
	memcpy(h, r, (n+1)*sizeof(uint));
	lcp(n+1, s, h, p);
	h[n] = 0;
	
	/*
	fprintf(stderr, "p = "); forn(i,n+1) fprintf(stderr, "%u ", p[i]); fprintf(stderr, "\n");
	fprintf(stderr, "r = "); forn(i,n+1) fprintf(stderr, "%u ", r[i]); fprintf(stderr, "\n");
	fprintf(stderr, "h = "); forn(i,n+1) fprintf(stderr, "%u ", h[i]); fprintf(stderr, "\n");
	*/
	double kval = klcp(n+1, h);
	printf("%s %.5lf\n", argv[1], kval);
	
	pz_free(r);
	pz_free(h);
	pz_free(p);
	return 0;
}
