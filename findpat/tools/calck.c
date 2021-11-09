#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bwt.h"
#include "lcp.h"
#include "klcp.h"
#include "tipos.h"
#include "macros.h"

uint *p, *r, *h;
uchar* s;
uint i;
double maxk;

double kval(uint n, uchar* s) {
	uint j;
	memcpy(p, s, n);
	bwt(NULL, p, r, NULL, n, NULL);
	forn(j,n) p[r[j]]=j;
	memcpy(h, r, n*sizeof(uint));
	lcp(n, s, h, p);
	h[n-1] = 0;
	return klcp(n, h)-1.0;
}

void generate() {
	double k;
	s[i] = 0;
	//printf("gen *%s* %d %d\n", s, i, j);
	k = kval(i+1, s);
	if (k > maxk) return;
	if (i) {
		printf("%s %lf %d\n", s, k, i);
	}
	++i;
	s[i-1]='0';
	generate();
	s[i-1]='1';
	generate();
	--i;
}

int main(int argc, char** argv) {
	uint maxn;
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <max_k>\n", argv[0]);
		return 1;
	}
	
	maxk = atof(argv[1]);
	maxn = pow(3.0,maxk+1)+2.0; //WARNING 3.0 should be e
	
	s = (uchar*)pz_malloc((maxn+1)*sizeof(uchar));
	p = (uint*)pz_malloc((maxn+1)*sizeof(uint));
	r = (uint*)pz_malloc((maxn+1)*sizeof(uint));
	h = (uint*)pz_malloc((maxn+1)*sizeof(uint));
	
	i=0;
	generate();
	
	pz_free(s);
	pz_free(r);
	pz_free(h);
	pz_free(p);
	return 0;
}
