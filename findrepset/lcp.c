#include "lcp.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "macros.h"


void lcp(uint n, uchar* s, uint* r, uint* p) {
	uint h = 0, i, j;
	forn(i,n) if (p[i] > 0) {
		j = r[p[i]-1];
		while(h < n && s[(i+h)%n] == s[(j+h)%n]) ++h;
		r[p[i]-1] = h;
		if (h > 0) --h;
	}
}
