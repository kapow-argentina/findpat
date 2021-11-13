#include "enc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "common.h"

#include "macros.h"

/**
 * Smaple .fa header

00000000  3e 38 20 64 6e 61 3a 63  68 72 6f 6d 6f 73 6f 6d  |>8 dna:chromosom|
00000010  65 20 63 68 72 6f 6d 6f  73 6f 6d 65 3a 4e 43 42  |e chromosome:NCB|
00000020  49 33 36 3a 38 3a 31 3a  31 34 36 32 37 34 38 32  |I36:8:1:14627482|
00000030  36 3a 31 0a                                       |6:1.|
00000034

 * Format 0x3e <string ascii> 0x0a
 */

#define ALPH_SZ 5
const uchar alph[ALPH_SZ] = {'A', 'C', 'G', 'N', 'T'};

#define _fa_enc(a, b, c, d) ((((a)*5+(b))*5+(c))*2+(((d)>>2)&1))
#define _fa_dec(v) (alph[(v)/(5*5*2)])

uint fa_encode(uchar* src, uchar* dst, uint n) {
	uint i;
	uint code[256];
	uint buf[4]; // frist 4;
	forn(i, 256) code[i] = -1;
	forn(i, ALPH_SZ) code[alph[i]] = i;

	if (!n) return n; // empty string
	forn(i, 4) buf[i] = src[i%n];
#define _fa_val(d) (code[((i+d)<n)?src[d]:buf[i+d-n]])
	for(i=0; i < n; ++i, ++src, ++dst) {
		if (code[*src] == -1) {
			fprintf(stderr, "Error: Invalid char found on FASTA stream: '%c' (0x%2X) at offset %u\n", *src, *src, i);
			return 0;
		}
		*dst = _fa_enc(_fa_val(0), _fa_val(1), _fa_val(2), _fa_val(3));
	}
	return n;
}

void fa_decode(uchar* src, uchar* dst, uint n) {
	while(n--) {
		*dst = _fa_dec(*src);
		++dst, ++src;
	}
}


uchar* fa_strip_header(uchar* src, uint n) {
	uchar* res = src;
	// Check for valid .fa header
	if (n && (src[0] == 0x3e)) {
		// Find first occurrence of 0x0a
		res = memchr(src, 0x0a, n);
		if (res == NULL) {
			fprintf(stderr, "Warning: FASTA header begin found, but never ends");
			return src;
		}
		res++; // Se come el enter
		fprintf(stderr, "Found FASTA header: ");
		if (!fwrite(src, 1, (size_t)(res-src), stderr)) perror("ERROR Writing to stderr");
	}
	return res;
}

uint fa_copy_cont(uchar* fasrc, uchar* dst, uint n) {
	uint res = 0;
	uchar* src = fa_strip_header(fasrc, n);
	n -= (src-fasrc);
	while(n--) {
		if ((*src != 0x0a)&&(*src != 'A')&&(*src != 'C')&&(*src != 'G')&&(*src != 'T')&&(*src != 'N')) {
			fprintf(stderr, "Warning: Invalid char found on FASTA stream: '%c' (0x%2X).\n", *src, *src);
		} else {
			if (*src != 0x0a) { *(dst++) = *src; res++; }
		}
		++src;
	}
	return res;
}

uint fa_strip_n(uchar* src, uchar* dst, uint n) {
	uint res = 0, c = 0;
	const uint minN = 2;
	for(;n--;++src) {
		if (*src == 'N') c++;
		else {
			if (c > minN) c = minN + bneed(c-minN);
			while(c) --c,++res,*(dst++) = 'N';
			++res,*(dst++)=*src;
		}
	}
	if (c > minN) c = minN + bneed(c-minN);
	while(c) --c,++res,*(dst++) = 'N';
	return res;
}

uint fa_strip_n_and_blanks(uchar* src, uchar* dst, uint n) {
	uint res = 0, c = 0;
	const uint minN = 2;
	for(;n--;++src) {
		if (*src == 'N') c++;
		else if (((*src)=='A')||((*src)=='C')||((*src)=='T')||((*src)=='G')) {
			if (c > minN) c = minN + bneed(c-minN);
			while(c) --c,++res,*(dst++) = 'N';
			++res,*(dst++)=*src;
		}
	}
	if (c > minN) c = minN + bneed(c-minN);
	while(c) --c,++res,*(dst++) = 'N';
	return res;
}

uint fa_strip_n_trac(uchar* src, uchar* dst, uint n, uint **trac_buf, uint *trac_size, uint lres) {
	uint res = 0, c = 0, t = 1, i, b = 0;
	uint *tb = NULL;
	const uint minN = 2;
	/* First pass: count */
	for(i=0;i<n;++i) {
		if (src[i] == 'N') c++;
		else { if (c >= 5) t++; c = 0; }
	}
	if (c >= 5) t++; c = 0;

	/* First pass: convert and trac */
	if (*trac_buf != NULL) {
		tb = (uint*)pz_malloc(sizeof(uint)*2*(*trac_size+t));
		memcpy(tb, *trac_buf, *trac_size * sizeof(uint) * 2);
		pz_free(*trac_buf);
		*trac_buf = tb;
		tb += *trac_size*2;
		*trac_size += t;
		b += tb[-1];
	} else {
		*trac_size = t;
		*trac_buf = tb = (uint*)pz_malloc(sizeof(uint)*2*t);
	}
	*(tb++) = lres; *(tb++) = b;

	for(;n--;++src) {
		if (*src == 'N') c++;
		else {
			if (c >= 5) { *(tb++) = lres+res+minN; *(tb++) = (b+=c-(minN + bneed(c-minN))); }
			if (c > minN) c = minN + bneed(c-minN);
			while(c) --c,++res,*(dst++) = 'N';
			++res,*(dst++)=*src;
		}
	}
	if (c >= 5) { *(tb++) = lres+res+minN; *(tb++) = (b+=c-(minN + bneed(c-minN))); }
	if (c > minN) c = minN + bneed(c-minN);
	while(c) --c,++res,*(dst++) = 'N';
	return res;
}

/* Converts a virtual position (Ns stripped) to a real position (with Ns) */
uint trac_convert_pos_virtual_to_real(uint x, uint* trac_buf, uint trac_size) {
	uint a = 0, b = trac_size, c;
	while (a + 1 < b) {
		c = (a + b) / 2;
		if (trac_buf[2 * c] <= x) a = c; else b = c;
	}
	return trac_buf[2 * a + 1] + x;
}

/* Converts a real position (with Ns) to a virtual position (Ns stripped)*/
uint trac_convert_pos_real_to_virtual(uint x, uint* trac_buf, uint trac_size) {
	uint a = 0, b = trac_size, c;
	while (a + 1 < b) {
		c = (a + b) / 2;
		if (trac_buf[2 * c] + trac_buf[2 * c + 1] <= x) a = c; else b = c;
	}
	return x - trac_buf[2 * a + 1];
}

