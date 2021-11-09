#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>

#include "macros.h"
#include "common.h"
#include "enc.h"
#include "bwt.h"
#include "lcp.h"
#include "mrs.h"
#include "lpat.h"
#include "nsrph.h"

/*
int *Acc = NULL;

void process_repeat(uint length, uint index, uint noccs, void* vout) {
	output_readable_data* out = (output_readable_data*)vout;
	uint j;
	forn (j, noccs) {
		Acc[out->r[index + j]]++;
		Acc[out->r[index + j] + length]--;
	}
}
*/

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s file.fa[.gz] {+,-}ml\n", argv[0]);
		fprintf(stderr, "Outputs the fragments of the input that are\n");
		fprintf(stderr, "covered by exact repeats of length {>=,<} ml\n");
		return 1;
	}
	char *fn = argv[1];
	uint ml = atoi(&argv[2][1]);
	assert(argv[2][0] == '+' || argv[2][0] == '-');
	int opt_geq = (argv[2][0] == '+');

	uint n; uchar *s; uint *r, *p;
	fasta_nsrph(fn, &n, &s, &r, &p, NULL);
	longest_mrs(n, s, r, p);

	int putsep = 0;
	uint i;
	forn (i, n) {
		int cond;
		if (opt_geq) {
			cond = p[i] >= ml;
		} else {
			cond = p[i] < ml;
		}
		if (cond) {
			printf("%c", s[i]);
			putsep = 1;
		} else if (putsep) {
			printf("N");
			putsep = 0;
		}
	}

	return 0;
}

