#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "tipos.h"
#include "common.h"
#include "macros.h"
#include "tiempos.h"
#include "sa_search.h"

#define SEP		((char)0xff)

/** Time measurement **/

double time_build_bwt;
double time_binsearch;
double time_occurrences;

void init_times() {
	time_build_bwt = 0;
	time_binsearch = 0;
	time_occurrences = 0;
}

void report(char *outfn, uint source_size, uint ntargets, uint max_target_size, uint chunk_size) {
	FILE *out;
	if (!fileexists(outfn)) {
		out = fopen(outfn, "w");
		fprintf(out,
			"source_size\t"
			"ntargets\t"
			"max_target_size\t"
			"chunk_size\t"
			"build_bwt\t"
			"binsearch\t"
			"occurrences\n");
	} else {
		out = fopen(outfn, "a");
	}
	fprintf(out, "%u\t%u\t%u\t%u\t%f\t%f\t%f\n",
			source_size,
			ntargets,
			max_target_size,
			chunk_size,
			time_build_bwt,
			time_binsearch,
			time_occurrences);
	fclose(out);
}

void usage(char *prog) {
	fprintf(stderr, "Usage: %s <source-file> <target-file>\n", prog);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -c <chunk_size>  size of the chunks in bytes\n");
	fprintf(stderr, "    -o <overlap>     overlap between chunks in bytes => should be greater\n");
	fprintf(stderr, "                     than the length of the longest target string\n");
	exit(1);
}

int main(int argc, char** argv) {
#define MAX_ARGS	2
	char *xargv[MAX_ARGS];
	uint xargc = 0;

	/* Defaults */
	uint overlap = 100;
	uint chunk_size = 1024 * 1024 * 1024;

	/* Read command line options */
	uint i;
	forsn (i, 1, argc) {
		if (!strcmp(argv[i], "-o")) {
			overlap = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-c")) {
			chunk_size = atoi(argv[++i]);
		} else {
			if (xargc >= MAX_ARGS) {
				usage(argv[0]);
			}
			xargv[xargc++] = argv[i];
		}
	}

	if (xargc != 2) usage(argv[0]);

	init_times();
	fprintf(stderr, "chunk_size %u\n", chunk_size);

	char *source_file = xargv[0]; 
	char *target_file = xargv[1]; 
	FILE *f = fopen(source_file, "r");
	if (!f) {
		fprintf(stderr, "cannot open \"%s\"\n", source_file);
		exit(1);
	}

	/* For each chunk */
	uint source_size = 0;
	uint ntargets = 0;
	uint max_target_size = 0;

	uint bufsize = chunk_size + overlap;
	char *s = (char *)pz_malloc((bufsize + 1) * sizeof(char));
	uint *p = (uint*)pz_malloc((bufsize + 1) * sizeof(uint));
	uint *r = (uint*)pz_malloc((bufsize + 1) * sizeof(uint));
	uint n = chunk_size + 1;
	uint nchunk = -1;
	while (n > chunk_size) {
		nchunk++;
		fprintf(stderr, "@@chunk %u\n ", nchunk);

		n = fread(s, sizeof(char), bufsize, f);
		if (n > chunk_size) {
			/* Rewind to chunk_size boundary */
			fseek(f, -(int)(n - chunk_size), SEEK_CUR);
			source_size += chunk_size;
		} else {
			source_size += n;
		}

		/* Build structure */
		s[n] = SEP;
		n++;
		MEASURE_TIME(time_build_bwt, {
			memcpy(p, s, n);
			bwt(NULL, p, r, NULL, n, NULL);
		});

		FILE *g = fopen(target_file, "r");
		if (!g) {
			fprintf(stderr, "cannot open \"%s\"\n", target_file);
			exit(1);
		}
		for (;;) {
			uint m;
			char *target = fileReadline(g, &m);
			if (!target) break;

			ntargets++;
			if (m > max_target_size) max_target_size = m;

			uint from, to;
			bool found;
			MEASURE_TIME(time_binsearch, {
				found = sa_search((uchar *)s, r, n, (uchar *)target, m, &from, &to);
			});
			if (!found) continue;

			uint noccs = 0;
			float time_this_occ = 0;
			MEASURE_TIME(time_this_occ, {
				/* Count the valid occurrences to report the number correctly */
				forsn (i, from, to) {
					if (r[i] >= chunk_size) continue;
						/* should find it in the next iteration */
					noccs++;
				}
			});
			time_occurrences += 2 * time_this_occ;

			printf("%s #%u (%u)\n ", target, noccs, m);
			forsn (i, from, to) {
				uint pos;
				if (r[i] >= chunk_size) continue;
					/* should find it in the next iteration */
				pos = r[i] + nchunk * chunk_size;
				printf(" <%d", pos);
			}
			printf("\n");

			pz_free(target);
		}
		fclose(g);
	}
	pz_free(s);
	pz_free(p);
	pz_free(r);
	fclose(f);

	report("chunksearch.txt", source_size, ntargets, max_target_size, chunk_size);

	return 0;
}

