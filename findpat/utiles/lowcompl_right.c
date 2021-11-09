#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "macros.h"
#include "common.h"
#include "enc.h"
#include "bwt.h"
#include "lcp.h"
#include "nsrph.h"

#define Lcp(I)	(h[r[(I)]])
#define inrg(X)	((X) < n)
#define Max_delta 1000000
void lowcompl(uint n, uchar *s, uint *r, uint *p, uint *h) {
	uint i;
	uint cambios = 0;
	for (i = 0; i < n;) {
		if (Lcp(i) == 0 || p[i] == n - 1) {
			i++;
		} else {
			uint j = i + Lcp(i);

			if (j >= n) break;

			uint tam_i = Lcp(i);
			uint tam_j = Lcp(j);
			uint delta = 0;

			uint q = r[p[i] + 1];

			while (tam_i + 1 < tam_j
					&& inrg(i + tam_i)
					&& inrg(q + tam_i)
					&& delta < Max_delta
					) {
				s[i + tam_i] = s[q + tam_i];
				tam_i++;
				tam_j--;
				delta++;
				cambios++;
			}
			i = j;
		}
	}

	fprintf(stderr, "tam: %u\n", n);
	fprintf(stderr, "cambios: %u\n", cambios);

	s[n - 1] = '\n';
	fwrite(s, 1, n, stdout);
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s file.fa[.gz]\n", argv[0]);
		return 1;
	}
	char *fn = argv[1];

	uint n; uchar *s; uint *r, *p, *h;
	fasta_nsrph(fn, &n, &s, &r, &p, &h);

	fprintf(stderr, "lowcompl\n");
	lowcompl(n, s, r, p, h);

	return 0;
}

