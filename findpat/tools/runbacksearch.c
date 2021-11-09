#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "bwt.h"
#include "tipos.h"
#include "common.h"
#include "macros.h"
#include "tiempos.h"
#include "comprsa.h"
#include "backsearch.h"

#define SEP		((char)0xff)
#define show_char(C)	((C) == SEP ? '$' : (C))

/** Time measurement **/

double time_build_bwt;
double time_build_backsearch;
double time_build_comprsa;
double time_lookup_backsearch;
double time_lookup_comprsa;

void init_times() {
	time_build_bwt = 0;
	time_build_backsearch = 0;
	time_build_comprsa = 0;
	time_lookup_backsearch = 0;
	time_lookup_comprsa = 0;
}

void report(char *outfn, uint source_size, uint ntargets, uint max_target_size) {
	FILE *out;
	if (!fileexists(outfn)) {
		out = fopen(outfn, "w");
		fprintf(out,
			"source_size\t"
			"ntargets\t"
			"max_target_size\t"
			"build_bwt\t"
			"build_backsearch\t"
			"build_comprsa\t"
			"lookup_backsearch\t"
			"lookup_comprsa\n");
	} else {
		out = fopen(outfn, "a");
	}
	fprintf(out, "%u\t%u\t%u\t%f\t%f\t%f\t%f\t%f\n",
			source_size,
			ntargets,
			max_target_size,
			time_build_bwt,
			time_build_backsearch,
			time_build_comprsa,
			time_lookup_backsearch,
			time_lookup_comprsa);
	fclose(out);
}

/** Search **/

void search(backsearch_data *backsearch, sa_compressed *comprsa, char *target, uint m) {
	uint i, from, to;
	MEASURE_TIME(time_lookup_backsearch, {
		backsearch_locate(backsearch, target, m, &from, &to);
	});
	printf("%s #%u (%u)\n ", target, to - from, m);
	forsn (i, from, to) {
		uint pos;
		MEASURE_TIME(time_lookup_comprsa, {
			pos = sa_lookup(comprsa, i);
		});
		printf(" <%d", pos);
	}
	printf("\n");
}

void usage(char *prog) {
	fprintf(stderr, "Usage: %s [options] <source> <target>\n", prog);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "       -f    use <source> and <target> as filenames instead of strings\n");
	exit(1);
}

void assert_in_alphabet(char *s, uint n) {
	uint i;
	forn (i, n) {
		if (backsearch_char_code(s[i]) == -1) {
			fprintf(stderr, "string should be in alphabet (%s)\n", Backsearch_alphabet);
			exit(1);
		}
	}
}

int main(int argc, char** argv) {
#define MAX_ARGS	2
	char *xargv[MAX_ARGS];
	uint xargc = 0;

	uint i;
	forsn (i, 1, argc) {
		if (xargc >= MAX_ARGS) {
			usage(argv[0]);
		}
		xargv[xargc++] = argv[i];
	}

	if (xargc != 2) usage(argv[0]);

	uint n;
	char *source = (char *)loadStrFileExtraSpace(xargv[0], &n, 2);
	if (!source) {
		fprintf(stderr, "couldn't read from file: %s\n", xargv[0]);
		exit(1);
	}
	assert_in_alphabet(source, n);
	source[n] = SEP;
	source[n + 1] = '\0';
	n++;

	init_times();

	/* Build structure */
	uint *p = (uint*)pz_malloc(n * sizeof(uint));
	uint *r = (uint*)pz_malloc(n * sizeof(uint));
	memcpy(p, source, n);

	fprintf(stderr, "@@@ start build_bwt\n");
	MEASURE_TIME(time_build_bwt, {
		bwt(NULL, p, r, NULL, n, NULL);
	});
	fprintf(stderr, "@@@ end build_bwt\n");

	backsearch_data backsearch;
	fprintf(stderr, "@@@ start build_backsearch\n");
	MEASURE_TIME(time_build_backsearch, {
		backsearch_init((char *)source, r, n, &backsearch);
	});
	fprintf(stderr, "@@@ end build_backsearch\n");

	pz_free(p);

	sa_compressed *comprsa = NULL;
	fprintf(stderr, "@@@ start build_comprsa\n");
	MEASURE_TIME(time_build_comprsa, {
		/* Compression frees r */
		comprsa = sa_compress((uchar *)source, r, n);
	});
	fprintf(stderr, "@@@ end build_comprsa\n");

	/* Search */
	uint ntargets = 0;
	uint max_target_size = 0;
	FILE *f = fopen(xargv[1], "r");
	for (;;) {
		uint m;
		char *target = fileReadline(f, &m);
		if (!target) break;

		ntargets++;
		if (m > max_target_size) max_target_size = m;

		assert_in_alphabet(target, m);
		search(&backsearch, comprsa, (char *)target, m);
		pz_free(target);
	}
	fclose(f);

	/* Destroy structures */
	sa_destroy(comprsa);
	backsearch_destroy(&backsearch);
	pz_free(source);

	report("backsearch.txt", n, ntargets, max_target_size);

	return 0;
}

