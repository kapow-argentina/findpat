#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "huffman.h"
#include "common.h"

#include "macros.h"


/*** bitstream ***/

/**
 * bstream puts first the LSB on the stream and later the MSB.
 */

#define BSTRM_CHUNK 16384


#define vector_push_back(v) { \
	if (nvec == zvec) { xvec = (uint64**)pz_malloc(2*zvec*sizeof(uint64*)); memset(xvec+zvec, 0, zvec * sizeof(uchar*)); memcpy(xvec, vec, zvec*sizeof(uchar*)); pz_free(vec); vec = xvec; zvec *= 2; } \
	vec[nvec++] = v; }

#define buf_rotate(size) { \
	xbuf = buf; zbuf = ((size)>2*zbuf)?(size):(2*zbuf); \
	buf = (uchar*)pz_malloc(sizeof(uchar)*zbuf); vector_push_back(buf); rbuf = zbuf; \
}

void obstream_create(obstream* this) {
	this->bused = 0;
	this->zvec = 32;
	this->vec = (uint64**)pz_malloc(sizeof(uint64*)* this->zvec);
	this->nvec = 1;
	this->vec[0] = this->buf = (uint64*)pz_malloc(BSTRM_CHUNK);
}

void obstream_destroy(obstream* this) {
	uint i;
	forn(i, this->nvec) pz_free(this->vec[i]);
	pz_free(this->vec);
	this->vec = 0; this->nvec = 0;
}

void obstream_write_bits(obstream* this, uint64 dat, uint bits) {
	uint64** xvec;
	/* puts the first part of the uint64 */
	uint b1 = ((this->bused & 63)+bits)>64?64-(this->bused & 63):bits;
	*(this->buf) |= (dat << (this->bused & 63)) & ((1 << b1)-1);
	bits -= b1;
	dat >>= b1;
	if ((this->bused & 63) == 0) this->buf++;
	if (this->bused == BSTRM_CHUNK*8) { /* Create new chunk */
		this->bused = 0;
		this->buf = (uint64*)pz_malloc(BSTRM_CHUNK);
		/* push-back new vector */
		if (this->nvec == this->zvec) {
			xvec = (uint64**)pz_malloc(this->zvec*2*sizeof(uint64*));
			memcpy(xvec, this->vec, this->zvec*sizeof(uint64*));
			pz_free(this->vec); this->vec = xvec;
			this->zvec *= 2;
		}
		this->vec[this->nvec++] = this->buf;
	}
	/* Puts the second part */
	if (bits) {
		*(this->buf) |= dat & ((1 << bits)-1);
		this->bused += bits;
	}
}

inline void obstream_write_byte(obstream* this, uint dat) {
	obstream_write_bits(this, dat, 8);
}

inline void obstream_write_uint(obstream* this, uint dat) {
	obstream_write_bits(this, dat, 32);
}

inline void obstream_write_ulong(obstream* this, uint64 dat) {
	obstream_write_bits(this, dat, 64);
}

void obstream_close(obstream* this) {
	uint padding = 7-(this->bused+2)%8;
	if (padding) obstream_write_bits(this, 0, padding);
	obstream_write_bits(this, padding, 3);
}

uint obstream_output_len(const obstream* this) {
	return (this->nvec-1)*BSTRM_CHUNK + (this->bused+7)/8;
}

uint obstream_output_copy(const obstream* this, uchar* dst) {
	uint i;
	forn(i, this->nvec-1) {
		memcpy(dst, this->vec[i], BSTRM_CHUNK);
		dst += BSTRM_CHUNK;
	}
	memcpy(dst, this->vec[this->nvec-1], (this->bused+7)/8);
	return (this->nvec-1)*BSTRM_CHUNK + (this->bused+7)/8;
}


/*** Huffman ***/

void huffman_calc_freqs(ushort* src, uint size, uint* freqs, uint alphaSize) {
	memset(freqs, 0, sizeof(uint) * alphaSize);
	while(size--) freqs[*(src++)]++;
}

#define heap_swap(a, b) { ord[a] ^= ord[b]; ord[b] ^= ord[a]; ord[a] ^= ord[b]; }
#define heap_downheap(i) { while (((hm=2*i+1)<n) && heap[ord[((hm+1<n) && heap[ord[hm+1]]<heap[ord[hm]])?++hm:hm]] < heap[ord[i]]) { heap_swap(i, hm); i = hm;} }
#define heap_upheap(i) { while (i && heap[ord[i]] < heap[ord[(hm=((i-1)/2))]]) { heap_swap(i, hm); i = hm;} }
#define heap_remove { if (n>1) heap_swap(0, n-1); n--; j = 0; heap_downheap(j); }
#define heap_sum(a, b) ((heap[a]+heap[b])&~(uint64)0xffff) | ((((heap[a]&0xffff)>(heap[b]&0xffff)?heap[a]:heap[b])+1) & 0xffff);

void huffman_code_lens(uint* freqs, uint* lens, uint alphaSize) {
	uint i, j, n = 0, m = alphaSize, hm, a, b;
	uint64* heap  = pz_malloc(alphaSize*2*sizeof(uint64));
	uint* parent = pz_malloc(alphaSize*2*sizeof(uint));
	uint* ord = pz_malloc((alphaSize+2)*sizeof(uint));
	/* heap */
	forn(i, alphaSize) { heap[i] = ((uint64)freqs[i] << 16LL) | 1; if (freqs[i]) ord[n++] = i; }
	/* heapify */
	dforn(j, n/2+1) heap_downheap(j);

	while (n >= 2) {
		a = ord[0]; heap_remove;
		b = ord[0]; heap_remove;
		heap[m] = heap_sum(a, b);
		parent[a] = parent[b] = m;
//		printf("(%3u, %3u) --> %3d  [%8llx]\n", a, b, m, heap[m]);
		ord[j = n++] = m++;
		heap_upheap(j);
	}

	forn(i, alphaSize) if (freqs[i]) {
		for(n = 1, j = i; j != ord[0]; j = parent[j], n++)
		lens[i] = n;
		if (n > HUFFMAN_MAX_CODELEN) { ppz_error("WTF? Hum? You're coding like a blind monkey! This never should happend, unless your input is bigger than fibonacci(%d), which is more than you can imagine.\n", HUFFMAN_MAX_CODELEN); exit(0); }
	} else lens[i] = 0;

//	DBGun(lens, alphaSize);

	pz_free(heap);
	pz_free(parent);
	pz_free(ord);
}

/* bit_reverse byte cache table */
const uchar bit_reverse8[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff};

inline uint64 bit_reverse(uint64 x) {
#define _bit_reverse8(x, i) ((uint64)bit_reverse8[(x >> (uint64)(i*8))&0xff] << (uint64)((7-i)*8))
	return
	  _bit_reverse8(x, 0) | _bit_reverse8(x, 1) | _bit_reverse8(x, 2) | _bit_reverse8(x, 3) |
	  _bit_reverse8(x, 4) | _bit_reverse8(x, 5) | _bit_reverse8(x, 6) | _bit_reverse8(x, 7);
#undef _bit_reverse8
}

void huffman_code_vals(uint* lens, uint64* codes, uint alphaSize) {
	uint i;
	uint64 ncod[HUFFMAN_MAX_CODELEN+1];

	memset(ncod, 0, sizeof(uint64)*(HUFFMAN_MAX_CODELEN+1));
	forn(i, alphaSize) ncod[lens[i]]++;
	forn(i, HUFFMAN_MAX_CODELEN) ncod[i+1] += ncod[i]*2;
	forn(i, alphaSize) codes[i] = lens[i]?bit_reverse(--ncod[lens[i]] << (uint64)(64-lens[i])):0;
	DBGun(lens, alphaSize);
	DBGlxn(codes, alphaSize);
}

uint64 huffman_compress_len(ushort* src, uint size, uint alphaSize, uint* tableSize) {
	uint64 res = 0, ucod;
	uint i, j, lc, bn, x, mx, sc;
	uint64 ncod[HUFFMAN_MAX_CODELEN+1];
	uint* freqs  = (uint*)pz_malloc(sizeof(uint) * alphaSize);
	uint* lens   = (uint*)pz_malloc(sizeof(uint) * alphaSize);
	huffman_calc_freqs(src, size, freqs, alphaSize);
	huffman_code_lens(freqs, lens, alphaSize);

	/* Calculates the length of the coding table */
	memset(ncod, 0, sizeof(uint64)*(HUFFMAN_MAX_CODELEN+1));
	forn(i, alphaSize) ncod[lens[i]]++;
	mx = 0;
	sc = 0; /* skip code: Don't write the list with most codes */
	forn(i, HUFFMAN_MAX_CODELEN+1) if (ncod[i]) { mx = i; if (ncod[i] > ncod[sc]) sc = i; }
	/* Write: mx in 2*bneed+1 */
	res += 2*bneed(mx)+1;
	if (mx == 1) {
		/* If mx = 1, write a bit indicating if ncod[1] is 1 or 2 */
		res += 1;
	} else {
		/* Write: the ncod[1..mx) vector in pseudo-fixed size, but don't write the last (ncod[mx]) (can be calculated) */
		ucod = 1; /* Free codes */
		forsn(i, 1, mx) {
			/* Write: ncod[i] using bneed(ucod) bits */
			res += bneed(ucod);
			ucod = 2*(ucod-ncod[i])+1;
		}
	}

	/* Write the list of codes for each length, writting each delta (starting from 0) in base 3.
	 * Do not write the list of length sc) */
	forn(i, mx+1) if (i != sc) {
		lc = -1;
		forn(j, alphaSize) if (lens[j] == i) {
			/* Write j-lc-1 in base 3 */
			bn = 1; /* one base-3 code for the number-terminator */
			x = j-lc;
			while (x) { bn++; x /= 3; }
			res += bn*2;
			lc = j;
		}
	}
	if (tableSize) *tableSize = res;

	/* Calculates the length of the compressed stream */
	forn(i, alphaSize) res += (uint64)lens[i] * (uint64)freqs[i];

	pz_free(lens);
	pz_free(freqs);
	return res;
}

void huffman_compress(obstream* obs, ushort* src, uint size, uint alphaSize) {
	uint64* codes =(uint64*)pz_malloc(sizeof(uint64) * alphaSize);
	uint* freqs  = (uint*)pz_malloc(sizeof(uint) * alphaSize);
	uint* lens   = (uint*)pz_malloc(sizeof(uint) * alphaSize);
	huffman_calc_freqs(src, size, freqs, alphaSize);
	huffman_code_lens(freqs, lens, alphaSize);
	huffman_code_vals(lens, codes, alphaSize);

	/* Write the coding tables */
	/* TODO: Escribir la tabla de huffman */

	/* Write the codes */
	while(size--) {
		obstream_write_bits(obs, codes[*src], lens[*src]);
		src++;
	}

	pz_free(lens);
	pz_free(freqs);
	pz_free(codes);
}
