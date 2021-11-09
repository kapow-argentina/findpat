#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "macros.h"
#include "common.h"
#include "sorters.h"
#include "enc.h"
#include "bwt.h"
#include "lcp.h"
#include "bitarray.h"
#include "bittree.h"
#include "nsrph.h"

void resumenes_cambios(uint n, uchar *s_original, uchar *s) {
	uint dist = 0;
	uint perdidas[256];
	uint ganadas[256];
	uint total_letras[256];
	uint i;

	forn (i, 256) perdidas[i] = 0;
	forn (i, 256) ganadas[i] = 0;
	forn (i, 256) total_letras[i] = 0;

	forn (i, n) {
		if (s_original[i] != s[i]) {
			dist++;
			perdidas[s_original[i]]++;
			ganadas[s[i]]++;
		}
		total_letras[s_original[i]]++;
	}
	char *letras = "ACGT";
	forn (i, 4) {
		fprintf(stderr, "cantidad de %c originales: %u\n", letras[i], total_letras[letras[i]]);
		fprintf(stderr, "cantidad de %c perdidas: %u\n", letras[i], perdidas[letras[i]]);
		fprintf(stderr, "cantidad de %c ganadas: %u\n", letras[i], ganadas[letras[i]]);
	}
	fprintf(stderr, "letras en total: %u\n", n);
	fprintf(stderr, "cantidad de cambios en total: %u (%.2f%%)\n", dist, 100*(float)dist/(float)n);
}

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s file1.fa[.gz] file2.fa[.gz]\n", argv[0]);
		return 1;
	}
	char *fn1 = argv[1];
	char *fn2 = argv[2];

	uint opt_strip = 0;

	uint n1, n2;
	uchar *s1, *s2;
	fasta_load(fn1, &n1, &s1, opt_strip);
	fasta_load(fn2, &n2, &s2, opt_strip);

#define MIN(x, y)	((x) < (y) ? (x) : (y))
	resumenes_cambios(MIN(n1, n2), s1, s2);

	return 0;
}
