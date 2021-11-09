#include <stdio.h>
#include <strings.h>

#include <assert.h>

#include "interv.h"
#include "macros.h"
#include "common.h"

interv *interv_new(uint n) {
	interv *in = pz_malloc(sizeof(*in) + sizeof(uint) * n);
	in->n = n;
	bzero(in->cov, sizeof(uint) * n);
	return in;
}

void interv_destroy(interv *in) {
	pz_free(in);
}

void interv_add(interv *in, uint a, uint b) {
	if (in->cov[a] < b - a) in->cov[a] = b - a;
}

void interv_union(interv *in, interv_callback cb) {
	uint i;
	const uint n = in->n;
	const uint *cov = in->cov;
	uint c = 0;
	forn (i, n) {
		if (cov[i] > c) {
			c = cov[i];
			cb(i, i + c);
		}
		if (c) --c;
	}
}

