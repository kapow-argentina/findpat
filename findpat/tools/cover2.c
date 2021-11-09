#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "common.h"
#include "macros.h"

char* strip_path(char* file) { //WARNING! Result is attached to original buffer
	int i = strlen(file);
	while(--i) if (file[i] == '/') return file+i+1;  //WARNING! Linux Only!
	return file;
}

char* make_joint_file(char* file1, char* file2, char* prefix, char* suffix) {
	int l1,l2,lp,ls;
	l1 = strlen(file1);
	l2 = strlen(file2);
	lp = strlen(prefix);
	ls = strlen(suffix);
	char* r = (char*)pz_malloc(l1+l2+lp+ls+2);
	memcpy(r, prefix, lp);
	memcpy(r+lp, file1, l1);
	memcpy(r+lp+l1+1, file2, l2);
	memcpy(r+lp+l1+l2+1, suffix, ls);
	r[lp+l1] = '-';
	r[l1+l2+lp+ls+1] = 0;
	return r;
}

int main(int argc, char* argv[]) {
	//uint i;
	int n,m;
	int *dif1, *dif2;
	char* pats;
	FILE* in;
	if (argc != 5) {
		fprintf(stderr, "Usage: %s <1st_chromosome> <2nd_chromosome> <pat_file_prefix> <pat_file_suffix>\n", argv[0]);
		return 1;
	}
	n = filesize(argv[1]);
	if (n < 0) {
		fprintf(stderr, "File %s could not be opened\n", argv[1]);
		return 1;
	} else {
		fprintf(stderr, "Opened %s of size %d\n", argv[1], n);
	}
	m = filesize(argv[2]);
	if (m < 0) {
		fprintf(stderr, "File %s could not be opened\n", argv[2]);
		return 1;
	} else {
		fprintf(stderr, "Opened %s of size %d\n", argv[2], m);
	}
	pats = make_joint_file(strip_path(argv[1]), strip_path(argv[2]), argv[3], argv[4]);
	in = fopen(pats, "rt");
	if (!in) {
		fprintf(stderr, "Pattern file %s could not be opened, trying reverse\n", pats);
		pats = make_joint_file(strip_path(argv[2]), strip_path(argv[1]), argv[3], argv[4]);
		in = fopen(pats, "rt");
		if (!in) {
			fprintf(stderr, "Pattern file %s could not be opened\nNo patterns found, closing\n", pats);
			pz_free(pats);
			return 1;
		}
	}
	pz_free(pats);
	//TODO: reemplazar n y m por los n y m reales buscados en db-srv
	dif1 = (int*)pz_malloc(n * sizeof(int));
	dif2 = (int*)pz_malloc(m * sizeof(int));
	pz_free(dif1);
	pz_free(dif2);
	fclose(in);
	return 0;
}
