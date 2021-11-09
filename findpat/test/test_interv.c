#include <stdio.h>
#include <stdlib.h>

#include "interv.h"

void print_interval(uint start, uint end) {
	printf("%i\t%i\n", start, end);
}

int main(int argc, char **argv) {

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <max>\n", argv[0]);
		fprintf(stderr, " reads intervals from stdin in the range [0, <max>)\n");
		fprintf(stderr, " and prints their union in stdout.\n"
				" format: one interval per line:  %%i\\t%%i\n");
		exit(1);
	}

	int mx = atoi(argv[1]);
	int start, end;
	interv *in = interv_new(mx);
	while (fscanf(stdin, "%i\t%i\n", &start, &end) == 2) {
		interv_add(in, start, end);
	}
	interv_union(in, print_interval);
	return 0;
}

