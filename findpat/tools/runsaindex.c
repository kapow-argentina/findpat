#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "saindex.h"
#include "tipos.h"
#include "macros.h"

int main(int argc, char** argv) {
	sa_index sa;
	uint n, m;
	uchar *s, *q;
	FILE* fp;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <string> <substring>\n", argv[0]);
		return 1;
	}
	
	/* Read parameters */
	s = (uchar*)argv[1];
	q = (uchar*)argv[2];
	n = strlen((char*)s);
	m = strlen((char*)q);

	saindex_init(&sa, s, n, 2, SA_DNA_ALPHA | SA_STORE_LCP);

	// saindex_show(&sa, stderr);
	fp = fopen("sa_index.dat", "wb");
	saindex_store(&sa, fp);
	fclose(fp);

	saindex_destroy(&sa);

	fp = fopen("sa_index.dat", "rb");
	saindex_load(&sa, fp);
	fclose(fp);

	saindex_show(&sa, stderr);

	/* Searches for the substring using both algorithms */
	uint from=0, to=0;
	bool found;

	/*
	found = saindex_mmsearch(&sa, q, m, &from, &to);
	fprintf(stderr, "Looking for «%s»\n", q);
	fprintf(stderr, "Using O(m*log(n)): %d [%d, %d)\n", found, from, to);
	* */

	from=0, to=0;
	found = saindex_mmsearch(&sa, q, m, &from, &to);
	fprintf(stderr, "Using O(m+log(n)): %d [%d, %d)\n", found, from, to);
	
	saindex_destroy(&sa);

	memset(&sa, 0, sizeof(sa));
	
	return 0;
}
