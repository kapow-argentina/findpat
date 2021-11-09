#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "macros.h"
#include "common.h"
#include "enc.h"
#include "bwt.h"
#include "lcp.h"
#include "mrs.h"

uint STRIP = 0;
void fasta_nsrph(char *fn, uint *pn, uchar **ps, uint **pr, uint **pp, uint **ph) {
	bool gzipped = FALSE;
	int l = strlen(fn);
	if (l > 3 && !strcmp(&fn[l - 3], ".gz")) {
		gzipped = TRUE;
	}

	uint length = 0;
	uchar *content = gzipped ? loadStrGzFile(fn, &length) : loadStrFile(fn, &length);
	if (!content) {
		fprintf(stderr, "Cannot open %s for reading\n", fn);
		exit(1);
	}

	uchar *real_content = fa_strip_header(content, length);
	uint real_length = length - (real_content - content);
	uint final_length = real_length;
	if (STRIP) {
		final_length = fa_strip_n_and_blanks(real_content, real_content, real_length);
	}

	/* Calculate Suffix Array and LCP */

	uchar *s = real_content;
	uint n = final_length;
	s[n] = 255;
	n++;

	uint *r = malloc(sizeof(uint) * n);
	uint *p = malloc(sizeof(uint) * n);

	fprintf(stderr, "bwt\n");
	bwt(NULL, p, r, s, n, NULL);

	uint i;
	forn (i, n) { p[r[i]] = i; }

	if (ph != NULL) {
		uint *h = malloc(sizeof(uint) * n);
		memcpy(h, r, sizeof(uint) * n);
		fprintf(stderr, "lcp\n");
		lcp(n, s, h, p);
		ph = h;
	}

	*pn = n;
	*ps = s;
	*pr = r;
	*pp = p;
}

void recalc_nsrph(uint n, uchar *s, uint *r, uint *p, uint *h) {
	bwt(NULL, p, r, s, n, NULL);
	uint i;
	forn (i, n) { p[r[i]] = i; }
	memcpy(h, r, sizeof(uint) * n);
	lcp(n, s, h, p);
}

void fasta_load(char *fn, uint *pn, uchar **ps, uint strip) {
	bool gzipped = FALSE;
	int l = strlen(fn);
	if (l > 3 && !strcmp(&fn[l - 3], ".gz")) {
		gzipped = TRUE;
	}

	uint length = 0;
	uchar *content = gzipped ? loadStrGzFile(fn, &length) : loadStrFile(fn, &length);
	if (!content) {
		fprintf(stderr, "Cannot open %s for reading\n", fn);
		exit(1);
	}

	uchar *real_content = fa_strip_header(content, length);
	uint real_length = length - (real_content - content);
	uint final_length = real_length;
	if (strip) {
		final_length = fa_strip_n_and_blanks(real_content, real_content, real_length);
	}

	*pn = final_length;
	*ps = real_content;
}

