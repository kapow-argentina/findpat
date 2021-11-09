#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "lcp.h"
#include "sa_search.h"
#include "tipos.h"
#include "macros.h"

int main(int argc, char** argv) {
	uint *p, *r, *h, *llcp, *rlcp;
	uchar *s, *q;
	uint n,m,i;
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <string> <substring>\n", argv[0]);
		return 1;
	}

	/* Read parameters */
	s = (uchar*)argv[1];
	q = (uchar*)argv[2];
	n = strlen((char*)s)+1;
	m = strlen((char*)q);

	s[n-1] = (char)0;
	
	p = (uint*)pz_malloc(n*sizeof(uint));
	r = (uint*)pz_malloc(n*sizeof(uint));
	h = (uint*)pz_malloc(n*sizeof(uint));
	llcp = (uint*)pz_malloc(n*sizeof(uint));
	rlcp = (uint*)pz_malloc(n*sizeof(uint));
	memcpy(p, s, n);
	bwt(NULL, p, r, NULL, n, NULL);
	forn(i,n) p[r[i]]=i;
	memcpy(h, r, n*sizeof(uint));
	lcp(n, s, h, p);
	h[n-1] = 0;

	/* Builds the aditional structures */
	build_lrlcp(n, h, llcp, rlcp);
	fprintf(stderr, "  r   h llcp rlcp\n");
	forn(i, n) {
		fprintf(stderr, "%3d %3d  %3d  %3d | %s\n", r[i], h[i], llcp[i], rlcp[i], s+r[i]);
	}

	/* Searches for the substring using both algorithms */
	uint from=0, to=0;
	bool found;
	
	found = sa_search(s, r, n, q, m, &from, &to);
	fprintf(stderr, "Looking for «%s»\n", q);
	fprintf(stderr, "Using O(m*log(n)): %d [%d, %d)\n", found, from, to);

	from=0, to=0;
	found = sa_mm_search(s, r, n, q, m, &from, &to, llcp, rlcp);
	
	fprintf(stderr, "Using O(m+log(n)): %d [%d, %d)\n", found, from, to);
	
	pz_free(p);
	pz_free(r);
	pz_free(h);
	pz_free(llcp);
	pz_free(rlcp);
	return 0;
}
