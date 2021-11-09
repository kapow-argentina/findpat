#include "mtf.h"

#include "macros.h"

// O(size * 256) temporal, mem = size + 256
void mtf_bf(uchar* src, uchar* dst, uint size) {
	uint l[256];
	uint i, j, z, x;

	forn(i, 256) l[i] = i;
	for(i = 0; i < size; ++i, ++src, ++dst) {
		z = l[0];
		for(j = 0; z != *src; ++j) {
			x = l[j+1]; l[j+1] = z; z = x;
		}
		l[0] = z;
		*dst = j;
	}
}

// O(size * 256) temporal, mem = size + 256
void imtf_bf(uchar* src, uchar* dst, uint size) {
	uint l[256];
	uint i, j;

	forn(i, 256) l[i] = i;
	for(i = 0; i < size; ++i, ++src, ++dst) {
		*dst = l[j = *src];
		while(j--) {
			l[j+1] = l[j];
		}
		l[0] = *dst;
	}
}

// O(size * sqrt(256)) temporal, mem = size + 256
void mtf_sqr(uchar* src, uchar* dst, uint size) {
	uint llp[256], lln[256]; // Circular linked lists nodes
	uint l[16]; // Linked lists
	uint inl[256]; // char to list
	uint i, j, zh, zl;

#define _mtf_agregarad(l, p) { lln[p] = l; llp[p] = llp[l]; llp[l] = p; lln[llp[p]] = p; l = p; }
#define _mtf_agregarat(l, p) { lln[p] = l; llp[p] = llp[l]; llp[l] = p; lln[llp[p]] = p; }
#define _mtf_sacar(l, p) { if (l == p) l = lln[p]; lln[llp[p]] = lln[p]; llp[lln[p]] = llp[p];}

	forn(i, 256) {
		lln[i] = (i & 0xF0) | ((i+1) & 0x0F);
		llp[i] = (i & 0xF0) | ((i+15) & 0x0F);
		inl[i] = i / 16;
	}
	forn(i, 16) l[i] = i*16;

	for(i = 0; i < size; ++i, ++src, ++dst) {
		zh = inl[*src];
		zl = 0;
		for(j = l[zh]; j != *src; j = lln[j]) zl++;
		*dst = zh*16 + zl; // Escribe la salida
		_mtf_sacar(l[zh], j);
		_mtf_agregarad(l[0], j); inl[j] = 0;
		forn(j, zh) { // Mueve el ultimo de l[j] al principio de l[j+1]
			zl = llp[l[j]];
			_mtf_sacar(l[j], zl);
			_mtf_agregarad(l[j+1], zl); inl[zl] = j+1;
		}
	}

/* // Muestra el estado de las 16 listas
	forn(i, 16) { printf("{ ");
		j = l[i];
		do {
			printf("%d ", j);
			j = lln[j];
		} while (j != l[i]);
		printf("}\n");
	}
*/
#undef _mtf_agregarad
#undef _mtf_agregarat
#undef _mtf_sacar
}

// O(size * log(MTF_SIZE)) temporal, mem = size + MTF_SIZE*2
uint mtf_log(uchar* src, uchar* dst, uint n) {
#define MTF_PW 10
#define MTF_SIZE (1<<MTF_PW)
	uint mem[2*MTF_SIZE];
	uint lru[256];
	uint i, j, k, r, res=0;
	#define _mtf_pone(x) { k = MTF_SIZE + (x); while(k) mem[k]++, k/=2; }
	#define _mtf_saca(x) { k = MTF_SIZE + (x); while(k) mem[k]--, k/=2; }
	#define _mtf_sacacuenta(x) { k = MTF_SIZE + (x); r = 0; while(k) { if ((k&1) == 0) r += mem[k+1]; mem[k]--; k/=2; } }
	#define _mtf_init_mem { \
		forn(j, MTF_SIZE) mem[MTF_SIZE+j] = j < 256; \
		dforn(j, MTF_SIZE) mem[j] = mem[2*j] + mem[2*j+1]; }
	#define _mtf_show { \
		forn(k, 256) printf("%d ", lru[k]); printf("\n"); \
		forn(k, 300) printf("%d", mem[MTF_SIZE+k]); printf("\n"); \
	}

	forn(i, 256) lru[i] = 255-i;

	_mtf_init_mem;
	j = 256;
	dforn(i, n) {
		if (j == MTF_SIZE) { // Re-sort
			k = 0;
			forn(j, MTF_SIZE) if (mem[MTF_SIZE+j]) mem[MTF_SIZE+j] = k++;
			forn(j, 256) lru[j] = mem[MTF_SIZE+lru[j]];
			_mtf_init_mem;
			j = 256;
		}
		_mtf_sacacuenta(lru[*src]);
		lru[*src] = j;
		*dst = r;
		res += (r == 0);
		_mtf_pone(j++);

//		_mtf_show; // return;

		src++, dst++;
	}
	return res;
}
