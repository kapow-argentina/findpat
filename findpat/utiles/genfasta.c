#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "common.h"
#include "macros.h"

#define BUFS 1024
char buf[BUFS];

#define NRATIO 5

#define rnd(N)		((int)((double)(N) * ((double)rand() / ((double)RAND_MAX + 1.0))))
#define min(X, Y)	(((X) < (Y)) ? (X) : (Y))

void usage(char *pn) {
	fprintf(stderr,
	"Usage: %s <size> [<pattern-file>] [<options>]\n"
	"\tGenerates an output of <size> bytes.\n"
	"\tIf no pattern-file is given, it uniformly produces strings from the alphabet.\n"
	"\tOtherwise, it randomly prints sections of the pattern-file\n"
	"\tinterleaved with N gaps.\n"
	"Options:\n"
	"\t-a <string>\t\tUses <string> as the alphabet. Default is ACTG\n"
	"\t-p <file>\t\tTakes the patterns from the file\n"
	, pn);
}

int main(int argc, char **argv) {
	int size;
	FILE *input;
	int input_size;
	int xargc = 0;
	char *xargv[1] = { NULL };
	char *bases = "ACTG";
	char *patfile = NULL;

	uint i;
	forsn (i, 1, argc) {
		if (0);
		else cmdline_opt_2(i, "-a") { bases = argv[i]; }
		else cmdline_opt_2(i, "-p") { patfile = argv[i]; }
		else if (cmdline_is_opt(i)) {
			fprintf(stderr, "%s: Unknown option: %s\n", argv[0], argv[i]);
			return 1;
		} else {
			if (xargc == 1) {
				fprintf(stderr, "%s: Too many arguments: %s\n", argv[0], argv[i]);
				usage(argv[0]);
				return 1;
			}
			xargv[xargc++] = argv[i];
		}
	}
	if (xargc != 1) {
		usage(argv[0]);
		return 1;
	}
	size = atoi(xargv[0]);

	int nbases = strnlen(bases, 1024);

	if (patfile) {
		if (!(input = fopen(patfile, "r"))) {
			fprintf(stderr, "Cannot read file: \"%s\".\n", patfile);
			return 1;
		}
		input_size = filesize(patfile);
	}

	srand(time(NULL));

	if (!patfile) {
		while (size-- > 0)
			putchar(bases[rnd(nbases)]);
	} else {
		int chunk_size;

		fprintf(stdout, ">header\n");
		for (;;) {
			chunk_size = rnd(BUFS);
			if (rnd(100) < NRATIO) {
				while (chunk_size-- > 0 && size-- > 0)
					putchar('N');
			} else {
				fseek(input, rnd(input_size - chunk_size), SEEK_SET);
				if (fread(buf, sizeof(char), chunk_size, input) != chunk_size) {
					perror("cannot read bytes from stream");
				}
				if (fwrite(buf, sizeof(char), min(chunk_size, size), stdout) != min(chunk_size, size)) {
					perror("cannot write bytes to stream");
				}
				size -= chunk_size;
				if (size <= 0) break;
			}
		}
	}

	if (patfile)
		fclose(input);
	return 0;
}
