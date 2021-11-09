#include "arit_enc.h"

#include "macros.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

uint arit_enc(ushort* src, uint n, uchar* dst) {
	memcpy(dst, src, n);
	return n;
}

uint arit_dec(ushort* src, uint n, uchar* dst) {
	memcpy(dst, src, n);
	return n;
}

uint arit_enc_est(ushort* src, uint n) {
	return ((uint)ceil(entropy_short(src, n)*n)+7)/8;
}

double entropy_char(uchar* src, uint n) {
	uint i;
	double r=0.0;
	uint* bc = (uint*)pz_malloc(256*sizeof(uint));
	memset(bc, 0, 256*sizeof(uint));
	forn(i,n) bc[src[i]]++;
#define p(i) (((double)bc[i])/((double)n))
	forn(i,256) if (bc[i]) r-=log2(p(i))*p(i);
#undef p
	pz_free(bc);
	return r;
}

double entropy_short(ushort* src, uint n) {
	uint i;
	double r=0.0;
	uint* bc = (uint*)pz_malloc(256*256*sizeof(uint));
	memset(bc, 0, 256*256*sizeof(uint));
	forn(i,n) bc[src[i]]++;
#define p(i) (((double)bc[i])/((double)n))
	forn(i,256*256) if (bc[i]) r-=log2(p(i))*p(i);
#undef p
	pz_free(bc);
	return r;
}
