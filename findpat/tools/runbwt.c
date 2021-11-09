#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "tipos.h"
#include "macros.h"

int main(int argc, char** argv) {
	uint *p, *r;
	uint n,i,j;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <string>\n", argv[0]);
		return 1;
	}
	n = strlen(argv[1]);
	p = (uint*)pz_malloc(n*sizeof(uint));
	r = (uint*)pz_malloc(n*sizeof(uint));
	memcpy(p, argv[1], n);
	bwt(NULL, p, r, NULL, n, NULL);
	forn(i,n) {
		printf("%3d ", r[i]);
		forsn(j,r[i],n) printf("%c",argv[1][j]); printf("\n");
	}
	pz_free(r);
	pz_free(p);
	return 0;
}
