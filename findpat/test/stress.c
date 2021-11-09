#include "bwt.h"
#include "tiempos.h"
#include "macros.h"

#include <sys/utsname.h>
#include <stdlib.h>
#include <stdio.h>

#define MB (1024*1024)

#define LOGFILE "stress.log"

void log_mark(int size, int alph, double time) {
	struct utsname buf;
	if (uname(&buf) == -1) { perror("uname"); return; }
	FILE* f = fopen(LOGFILE, "at");
	if (!f) {
		fprintf(stderr, "Error loggin file\n");
		return;
	}
	fprintf(f, "rev%3.3d %5d MB %8.2lf s %3d %s-%s\n", SVNREVISION, size, time / 1000.0, alph, buf.nodename, buf.machine);
	fclose(f);
}

void test_sort(uint n, uint alph) {
	tiempo t1, t2;
	uint *p, *r;
	double tm;
	uint i;
	p = (uint*)pz_malloc(n*sizeof(uint));
	r = (uint*)pz_malloc(n*sizeof(uint));
	forn(i,n) *(((uchar*)p)+i) = rand()%alph;
	printf("Running %d bytes, alph=[0,%3d)...", n, alph); fflush(stdout);
	getTickTime(&t1);
	bwt(NULL, p, r, NULL, n, NULL);
	getTickTime(&t2);
	tm = getTimeDiff(t1, t2);
	printf(" [Done] Time consumed: %.2f s\n", tm/1000.0); fflush(stdout);
	pz_free(p);
	pz_free(r);
	log_mark(n/MB, alph, tm);
}

#define WRONG_PAR \
	{fprintf(stderr, "Usage: %s <input_megabytes> [<alpha_size>]\n", argv[0]);return 1;}

int main(int argc, const char** argv) {
	uint n, ch = 256;
	if (argc < 2) WRONG_PAR

	if (argc == 3) ch = atoi(argv[2]);

	n = atoi(argv[1]);
	n*=MB;
	test_sort(n, ch);

	return 0;
}
