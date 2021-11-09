#include <stdio.h>
#include <stdlib.h>
#include "tipos.h"
#include "bitarray.h"
#include "macros.h"

typedef int Bool;

typedef uint64 Kmer;

#define codechar(X) ( \
	(X) == 0 ? 'A' : \
	(X) == 1 ? 'C' : \
	(X) == 2 ? 'G' : \
	(X) == 3 ? 'T' : \
	0xff )
#define charcode(X) ( \
	(X) == 'A' ? 0 : \
	(X) == 'C' ? 1 : \
	(X) == 'G' ? 2 : \
	(X) == 'T' ? 3 : \
	0 )
#define kmer_at(kmer, k, i)	(((kmer) >> (2 * ((k) - (i) - 1))) & 0x3)
#define kmer_truncate(kmer, k)	((kmer) & (((uint64)1 << (2 * (k))) - 1))
#define kmer_add(kmer, base)	(((kmer) << 2) | (base))

#define _4pow(X)	((uint64)1 << (2 * (X)))

int print(Kmer kmer, uint k) {
	uint i;
	forn (i, k) {
		printf("%c", codechar(kmer_at(kmer, k, i)));
	}
}

int _sgte[4] = {0,1,2,3};

#define sgte_base(base, invertirp) ((invertirp) ? (3 - _sgte[(base)]) : _sgte[(base)])

uint generar_high_mtf_entropy(const uint k) {
	const uint64 n_kmers = _4pow(k);
	uint cant_cortes = 0;
	bitarray *usado = bita_malloc(n_kmers);
	bita_clear(usado, n_kmers);

	for (Kmer curr = 0; curr < n_kmers; curr++) {
		Kmer anterior = curr;
		uint iter = 0;
		for (;;) {
			if (bita_get(usado, anterior)) {
				if (iter > 0) cant_cortes++;
				break;
			}
			bita_set(usado, anterior);

			if (iter == 0) {
				print(anterior, k);
			}
			iter++;

			const Kmer base_anterior = kmer_at(anterior, k, 0);
			const Kmer centro = kmer_truncate(anterior, k - 1);
			//print(anterior, k); printf("\n");
			//print(base_anterior, 1); printf(" ++ "); print(centro, k - 1); printf("\n");
			const Kmer base_siguiente = sgte_base(base_anterior, centro % 2);
			const Kmer siguiente = kmer_add(centro, base_siguiente);
			//printf("==> "); print(siguiente, k); printf("\n");
			printf("%c", codechar(base_siguiente));
			anterior = siguiente;
		}
	}
	pz_free(usado);
	fprintf(stderr, "  TOTAL: %llu;\t", n_kmers);
	fprintf(stderr, "  CORTES: %u\n", cant_cortes);
	return cant_cortes;
}

int buena[4];
uint cortes_buenos = -1;
void generar_perms(const uint k, int p) {
	uint i, j1, j2;

	if (p == 4) {
		fprintf(stderr, "=== %u %u %u %u\n", _sgte[0], _sgte[1], _sgte[2], _sgte[3]);
		uint cortes = generar_high_mtf_entropy(k);
		if (cortes < cortes_buenos) {
			forn (i, 4) buena[i] = _sgte[i];
			cortes_buenos = cortes;
		}
		return;
	}
	forn (i, 4) {
		_sgte[p] = i;

		Bool sirve = 1;
		forn (j1, p+1) forn (j2, p+1) {
			if (j1 != j2 && _sgte[j1] == _sgte[j2]) {
				sirve = 0;
			}
		}
		if (!sirve) continue;
		generar_perms(k, p + 1);
	}
	if (p == 0) {
		fprintf(stderr, "MEJOR:\n");
		fprintf(stderr, "=== %u %u %u %u\n", buena[0], buena[1], buena[2], buena[3]);
		fprintf(stderr, "=== cortes_buenos %u\n", cortes_buenos);
	}
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <k>\n", argv[0]);
		return 1;
	}
	const uint k = atoi(argv[1]);

	_sgte[0] = 2;
	_sgte[1] = 0;
	_sgte[2] = 1;
	_sgte[3] = 3;
	/*generar_perms(k, 0);*/
	generar_high_mtf_entropy(k);
	return 0;
}

