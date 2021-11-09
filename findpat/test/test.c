#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "tipos.h"
#include "tiempos.h"

#include "common.h"
#include "mtf.h"
#include "bwt.h"
#include "sorters.h"
#include "rle.h"
#include "lzw.h"
#include "bwt_joint.h"
#include "huffman.h"
#include "lcp.h"
#include "bitarray.h"
#include "bittree.h"
#include "rmq.h"
#include "rmqbit.h"
#include "longpat.h"
#include "mrs.h"
#include "bitstream.h"
#include "klcp.h"
#include "sa_search.h"

#include "macros.h"

// Datos de prueba
const char* tcases[] = {
		"x","xx","xy","yx","mississipi","abcdabcdabcdabcd",
		"abcdabcdabcdabcdefgh","aaaaaaaaaaaaaaa","aaaaaaaaaaaab","baaaaaaaa",
		"abababababab","aaaaabaaaaabaaaaabaaaaab",
		"abcdefghijklmnopqrstuvwxyz",
		"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
		"aba","aab","baa","abc","bca","acb","bac","cab","cba",
		"abaaba","aabaab","baabaa","abcabc","bcabca","acbacb","bacbac",
		"cabcab","cbacba",
		"abaabaaba","aabaabaabaab","baabaabaabaa","abcabcabcabc",
		"bcabcabcabca","acbacbacbacb","bacbacbacbacbac","cabcabcabcab",
		"cbacbacbacba","abaabaaba","aabaabaabaaba","baabaabaabaaba",
		"abcabcabcabca","bcabcabcabcab","acbacbacbacbac","bacbacbacbacbacb",
		"cabcabcabcabc","cbacbacbacbac",
		"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyza",
		"zxywvutsrqponmlkjihgfedcbazxywvutsrqponmlkjihgfedcba",
		"zxywvutsrqponmlkjihgfedcbaabcdefghijklmnopqrstuvwxyz",
		"unkcvxyvwfxerpwhgwwonatogeuyaouudgwbdwwb",
		"cdccdcbddbcbcbdbaecccdcdddcbcdeac",
		"rrrllleee", "rrrrlllleeee", "rrrrrleeeee",
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbccccde",
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", // 259a
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", // 260a
		"abcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabc"
		"abcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabcaabcabcabcabc"
		};

/** Funciones de Testing de tiempos **/
#define time_init tiempo t1, t2;
#define time_run(X) { getTickTime(&t1); { X; } getTickTime(&t2); }
#define run_test(R, T) { time_run(R);  \
  if (T) printf("PASSED %6.0f ms\n", getTimeDiff(t1, t2)); \
  else printf("FAIL   %6.0f ms\n", getTimeDiff(t1, t2)); }

typedef void(fvoid)(void);
typedef int(functester)(uchar*, uint, double*, fvoid, fvoid);

int testCase(uchar* tcase, int n, functester frt, fvoid fr, fvoid ft, int verb, double* d) {
	int res;
	double _d;
	if (d == NULL) d = &_d;
	*d = 0;
	if ((res = frt(tcase, n, d, fr, ft))) {
		if (verb) printf("PASSED %6.0f ms\n", *d);
		else { printf("."); fflush(stdout); }
	} else {
		if (verb) printf("FAIL   %6.0f ms\n", *d);
		else { printf("F"); fflush(stdout); }
	}
	return res;
}

int testAll(char* testname, char** suite, int n, functester frt, fvoid fr, fvoid ft, int verb) {
	uint i;
	int res = 1;
	double dd;
	printf("Testing %s\n", testname);
	if (!verb) printf("[");
	forn(i, n) {
		if (verb) printf("Test %2d/%2d: ", i+1, n);
		res = testCase((uchar*)suite[i],strlen(suite[i]), frt, fr, ft, verb, &dd) || res;
	}
	if (!verb) printf("]\n");
	return res;
}

uchar* randomCase(uint n, uint mod) {
	uchar* res = (uchar*)pz_malloc(n);
	while(n--) res[n] = rand()%mod;
	return res;
}

/** TEST_MTF begin **/
typedef void(mtf_run)(uchar*, uchar*, uint);
typedef int(mtf_test)(uchar*, uchar*, uint);

int mtf_tester(uchar* src, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	int res;
	uchar *out = pz_malloc(n);
	getTickTime(&t1);
	((mtf_run*)fr)(src, out, n);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = (*(mtf_test*)ft)(src, out, n);
	pz_free(out);
	return res;
}

int mtf_imtf(uchar* src, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	int res;
	uchar *out = pz_malloc(n);
	uchar *src2 = pz_malloc(n);
	getTickTime(&t1);
	(*(mtf_run*)fr)(src, out, n);
	(*(mtf_run*)ft)(out, src2, n);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = !memcmp(src, src2, n);
	pz_free(out);
	pz_free(src2);
	return res;
}

int check_mtf(uchar* src, uchar* dst, uint n) {
	uint ls[256], i, j, x;
	forn(i, 256) ls[i] = i;
	forn(i, n) {
		if (ls[dst[i]] != src[i]) return 0;
		x = ls[dst[i]];
		j = dst[i];
		while(j) (ls[j] = ls[j-1]), j--;
		ls[0] = x;
	}
	return 1;
}

void test_mtf() {
	int i, j;
	uint bign = 1*1024*1024;
	uchar *big = randomCase(bign, 256);
	testAll("MTF-sqr", (char**)tcases, sizeof(tcases)/sizeof(char*), mtf_tester, (fvoid*)mtf_sqr, (fvoid*)check_mtf, 0);
	printf("big test: "); testCase(big, bign, mtf_tester, (fvoid*)mtf_sqr, (fvoid*)check_mtf, 1, NULL);

	testAll("MTF-bf",  (char**)tcases, sizeof(tcases)/sizeof(char*), mtf_tester, (fvoid*)mtf_bf, (fvoid*)check_mtf, 0);
	printf("big test: "); testCase(big, bign, mtf_tester, (fvoid*)mtf_bf, (fvoid*)check_mtf, 1, NULL);

	testAll("MTF-log", (char**)tcases, sizeof(tcases)/sizeof(char*), mtf_tester, (fvoid*)mtf_log, (fvoid*)check_mtf, 0);
	printf("big test: "); testCase(big, bign, mtf_tester, (fvoid*)mtf_log, (fvoid*)check_mtf, 1, NULL);

	printf("MTF-log random test, %u bytes\n[", bign);
	forn(i, 50) {
		forn(j, bign) big[j] = rand()%256;
		testCase(big, bign, mtf_tester, (fvoid*)mtf_log, (fvoid*)check_mtf, 0, NULL);
	}
	forn(i, 50) {
		forn(j, bign) big[j] = 19*(rand()%8);
		testCase(big, bign, mtf_tester, (fvoid*)mtf_log, (fvoid*)check_mtf, 0, NULL);
	}
	printf("]\n");

	testAll("mtf_log/imtf_bf", (char**)tcases, sizeof(tcases)/sizeof(char*), mtf_imtf, (fvoid*)mtf_log, (fvoid*)imtf_bf, 0);
//	testAll("mtf_log/imtf_log", (char**)tcases, sizeof(tcases)/sizeof(char*), mtf_imtf, (fvoid*)mtf_log, (fvoid*)imtf_log, 0);

	pz_free(big);
}
/** TEST_MTF end **/

/** TEST_SORT begin **/
typedef void(sort_run)(ushort*, ushort*);
typedef int(sort_test)(ushort*, ushort*, uint); // begin_unsort, begin_sort, size

int check_sort(ushort* bu, ushort* bs, uint n) {
	uint fq[256], i;
	forn(i, 256) fq[i] = 0;
	forn(i, n) fq[(uchar)bu[i]]++, fq[(uchar)bs[i]]--;
	forn(i, 256) if (fq[i]) {
		fprintf(stderr, "Hay distinta cantidad del caracter %d '%c'\n", i, i);
		exit(0);
		return 0;
	}
	forn(i, n-1) if ((uchar)bs[i] > (uchar)bs[i+1]) {
//		printf("\n%s -->\n%s\n", bu, bs);
		exit(0);
		return 0;
	}
//	printf("%s\n", bs);
	return 1;
}

int sort_tester(uchar* b, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
//	uint desp = 0; // Desplazamiento del resultado para ibwt
	int res, i;
	ushort* bb = pz_malloc((n+1)*sizeof(ushort));
	ushort *buf = pz_malloc((n+1)*sizeof(ushort));
	forn(i, n) bb[i] = b[i] + (i << 8);
	memcpy(buf, bb, n*sizeof(ushort));
	buf[n] = 0;


	getTickTime(&t1);
	(*(sort_run*)fr)(buf, buf+n);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = (*(sort_test*)ft)(bb, buf, n);
	pz_free(bb);
	pz_free(buf);
	return res;
}

/* qsort_uint */
#define _VAL(x) (*(uchar*)(x))
_def_qsort3(qsort_uchar, ushort, uchar, _VAL, <)

void test_sort() {
	uint bign = 1024*1024;
	uint sman = 4*1024;
	uchar *big = pz_malloc(bign*sizeof(uchar));
	int i, j;
	testAll("SORT: qsort_uchar", (char**)tcases, sizeof(tcases)/sizeof(char*), sort_tester, (fvoid*)qsort_uchar, (fvoid*)check_sort, 0);

	printf("SORT random test, %u bytes\n[", bign);
	double tm, stm = 0;
	forn(i, 50) { // Stress Test
		forn(j, bign) big[j] = 'a' + rand()%26 + (j << 8);
		testCase(big, bign, sort_tester, (fvoid*)qsort_uchar, (fvoid*)check_sort, 0, &tm); stm += tm;
	}
	printf("] prom: %.0f ms\n[", stm / 50);
	forn(i, 1000) { // Stress Test
		forn(j, bign) big[j] = rand();
		testCase(big, 1+rand()%sman, sort_tester, (fvoid*)qsort_uchar, (fvoid*)check_sort, 0, &tm);
	}
	printf("]\n");
	pz_free(big);
}
/** TEST_SORT end **/

/** TEST_BWT begin **/
typedef void(bwt_run)(uchar*, uint*, uint*, uchar*, uint, uint*);
typedef void(bwt_inv)(uchar*, uchar*, uint, uint); //(uchar *src, uchar *dst, int n, int prim);
typedef int(bwt_test)(uint, uchar*, uchar*, uint*); // src original, str salida de la BWT, indices

int is_sorted(char* v[], int n) {
	int i;
	forn(i, n) if (strcmp(v[i], v[i+1]) > 0) return i;
	return -1;
}

void show_rot(uint n, uchar* src, uint* dst) {
	int i, j;
	printf("\n");
	forn(i, n) {
		printf("%3d ", dst[i]);
		forn(j, n) printf("%c", src[(dst[i]+j)%n]);
		printf("\n");
	}
}

int check_bwt(uint n, uchar* src, uchar* dst, uint* idx) {
	int i;
	uchar* mem = pz_malloc(2*n);
	memcpy(mem, src, n);
	memcpy(mem+n, src, n);
	// Verifica que las strings estén en orden
	forsn(i, 1, n) if (memcmp(mem+idx[i-1], mem+idx[i], n) > 0) {
		//printf("error: %d vs %d\n", idx[i-1], idx[i]);
		//show_rot(n, src, idx);
		/*ESTO LO COMENTE PORQUE SI NO IMPRIME UN TEST ENORME EN LA PANTALLA*/
		//exit(0);
		pz_free(mem);
		return 0;
	}
	// Verifica que la salida de la BWT sea la última columna
	forn(i, n) {
		if (dst[i] != mem[idx[i]+n-1]) {
			printf("En la posición %d de la salida de BWT hay un %c pero se esperaba %c\n", i, dst[i], mem[idx[i]+n-1]);
//			show_rot(n, src, idx);
			pz_free(mem);
			return 0;
		}
	}
	pz_free(mem);
	return 1;
}

int bwt_tester(uchar* src, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	uint desp = 0; // Desplazamiento del resultado para ibwt
	int res,i;
	uint *out = (uint*)pz_malloc(n * sizeof(uint));
	uchar *srcB = (uchar*)pz_malloc(n * sizeof(uint));
	forn(i,n) srcB[i] = src[i];
	getTickTime(&t1);
	(*(bwt_run*)fr)(NULL, (uint*)srcB, out, NULL, n, &desp);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = (*(bwt_test*)ft)(n, src, srcB, out); //, desp);
	pz_free(out);
	pz_free(srcB);
	return res;
}

int bwt_ibwt(uchar* src, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	uint desp = 0; // Desplazamiento del resultado para ibwt
	int res,i;
	getTickTime(&t1);
	uint *out = (uint*)pz_malloc(n * sizeof(uint));
	uchar *srcB = (uchar*)pz_malloc(n * sizeof(uint));
	uchar *out2 = (uchar*)pz_malloc(n * sizeof(uchar));
	forn(i,n) srcB[i] = src[i];

//	DBGcn(src, n);
	(*(bwt_run*)fr)(NULL, (uint*)srcB, out, NULL, n, &desp);
//	DBGu(desp);
//	DBGcn(srcB, n);
	(*(bwt_inv*)ft)(srcB, out2, n, desp);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
//	DBGcn(out2, n);
	res = !memcmp(src, out2, n);
	pz_free(out);
	pz_free(out2);
	pz_free(srcB);
	return res;
}

void test_bwt() {
	uint bign = 1024*1024;
	uint sman = 500;
	uchar *big = pz_malloc(bign);
	uint i, j, k, al, sz;
	uint als[5] = {2,6,8,128,254};
	uint szs[10] = {30,50,70,90,100,150,300,512,1024,bign-5};

	testAll("BWT", (char**)tcases, sizeof(tcases)/sizeof(char*), bwt_tester, (fvoid*)bwt, (fvoid*)check_bwt, 0);

	printf("BWT random test, %u bytes\n[", bign);
	forn(al,sizeof(als)/sizeof(uint))forn(sz,sizeof(szs)/sizeof(uint))forn(i, 5) { // Stress Test
		forn(j, bign) big[j] = 'a' + rand()%als[al];
		testCase(big, 1+rand()%szs[sz], bwt_tester, (fvoid*)bwt, (fvoid*)check_bwt, 0, NULL);
	}

	forn(i, 158) { // Stress Test
		k = 2+rand()%6;
		forn(j, bign) big[j] = 'a' + rand()%k;
		testCase(big, 1+rand()%bign, bwt_tester, (fvoid*)bwt, (fvoid*)check_bwt, 0, NULL);
	}


	printf("]\nBWT random test, 2^n-1, 2^n and 2^n+1 bytes\n[");
	forsn(i, 1, 15) { // 2^n+-1 length
		forn(k, 10) {
			forn(j, ((1<<i)+1)) big[j] = 'a' + rand()%6;
			testCase(big, (1<<i)-1, bwt_tester, (fvoid*)bwt, (fvoid*)check_bwt, 0, NULL);
			testCase(big, (1<<i), bwt_tester, (fvoid*)bwt, (fvoid*)check_bwt, 0, NULL);
			testCase(big, (1<<i)+1, bwt_tester, (fvoid*)bwt, (fvoid*)check_bwt, 0, NULL);
		}
	}
	printf("]\n");

	testAll("BWT/IBWT", (char**)tcases, sizeof(tcases)/sizeof(char*), bwt_ibwt, (fvoid*)bwt, (fvoid*)ibwt, 0);

	printf("BWT/IBWT random test, %u bytes\n[", bign);
	forn(i, 300) { // Stress Test
		forn(j, bign) big[j] = 'a' + rand()%5;
		testCase(big, 1+rand()%sman, bwt_ibwt, (fvoid*)bwt, (fvoid*)ibwt, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_BWT end **/

/** TEST_OBWT begin **/

void test_obwt() {
	uint bign = 1024*1024;
	uint sman = 500;
	uchar *big = pz_malloc(bign);
	uint i, j, k, al, sz;
	uint als[5] = {2,6,8,128,254};
	uint szs[10] = {30,50,70,90,100,150,300,512,1024,bign-5};

	testAll("OBWT", (char**)tcases, sizeof(tcases)/sizeof(char*), bwt_tester, (fvoid*)obwt, (fvoid*)check_bwt, 0);

	printf("OBWT random test, %u bytes\n[", bign);
	forn(al,sizeof(als)/sizeof(uint))forn(sz,sizeof(szs)/sizeof(uint))forn(i, 5) { // Stress Test
		forn(j, bign) big[j] = 'a' + rand()%als[al];
		testCase(big, 1+rand()%szs[sz], bwt_tester, (fvoid*)obwt, (fvoid*)check_bwt, 0, NULL);
	}

	forn(i, 158) { // Stress Test
		k = 2+rand()%6;
		forn(j, bign) big[j] = 'a' + rand()%k;
		testCase(big, 1+rand()%bign, bwt_tester, (fvoid*)obwt, (fvoid*)check_bwt, 0, NULL);
	}


	printf("]\nOBWT random test, 2^n-1, 2^n and 2^n+1 bytes\n[");
	forsn(i, 1, 15) { // 2^n+-1 length
		forn(k, 10) {
			forn(j, ((1<<i)+1)) big[j] = 'a' + rand()%6;
			testCase(big, (1<<i)-1, bwt_tester, (fvoid*)obwt, (fvoid*)check_bwt, 0, NULL);
			testCase(big, (1<<i), bwt_tester, (fvoid*)obwt, (fvoid*)check_bwt, 0, NULL);
			testCase(big, (1<<i)+1, bwt_tester, (fvoid*)obwt, (fvoid*)check_bwt, 0, NULL);
		}
	}
	printf("]\n");

	testAll("OBWT/IBWT", (char**)tcases, sizeof(tcases)/sizeof(char*), bwt_ibwt, (fvoid*)obwt, (fvoid*)ibwt, 0);

	printf("OBWT/IBWT random test, %u bytes\n[", bign);
	forn(i, 300) { // Stress Test
		forn(j, bign) big[j] = 'a' + rand()%5;
		testCase(big, 1+rand()%sman, bwt_ibwt, (fvoid*)obwt, (fvoid*)ibwt, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}

void test_obwt_cmp_size(uint bign) {
	uint i,j,k,n,c=30;
	uchar* big=(uchar*)pz_malloc(bign);
	double d,dj,avg=0.0;
	printf("OBWT vs BWT random test, %u bytes\n", bign);
	forn(i, c) { // Stress Test
		forn(j, bign) big[j] = 'a' + rand()%8;
		k = bign/2+(rand()%(bign/2+1)); /*Only test on really big inputs*/
		n = k/3+rand()%(k/3); /*Partition in similar sizes*/
		big[n] = 255;
		printf("OPTIMIZED BWT   test %u/%u: ", i+1, c);
		testCase(big, k, bwt_tester, (fvoid*)obwt, (fvoid*)check_bwt, 1, &dj);
		printf("NORMAL BWT test %u/%u: ", i+1, c);
		testCase(big, k, bwt_tester, (fvoid*)bwt, (fvoid*)check_bwt, 1, &d);
		avg+=dj/d;
		printf("*** SIZE=%u OPTIMIZED/NORMAL: %.4f AVG=%.3f***\n", k, dj/d, avg/(i+1));
	}
	pz_free(big);
}

void test_obwt_cmp() {
	test_obwt_cmp_size(2*1024*1024);
}

void test_obwt_cmp_big() {
	test_obwt_cmp_size(20*1024*1024);
}

void test_obwt_cmp_huge() {
	test_obwt_cmp_size(200*1024*1024);
}


/** TEST_OBWT end **/

/** TEST_RLE begin **/
typedef int(rle_run)(uchar*, uchar*, uint);
typedef int(rle_test)(uchar*, uchar*, uint);

int rle_irle(uchar* src, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	int res;
	uint rlez = rle_len(src, n), rlez2, irlez;
	uchar *out = pz_malloc(rlez);
	uchar *src2 = pz_malloc(n);
	getTickTime(&t1);
	rlez2 = (*(rle_run*)fr)(src, out, n);
	irlez = (*(rle_run*)ft)(out, src2, rlez);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = (rlez == rlez2) && (irlez == n) && (!memcmp(src, src2, n));
	pz_free(out);
	pz_free(src2);
	return res;
}

int check_rle(uchar* src, uchar* dst, uint n) {
	uint ls[256], i, j, x;
	forn(i, 256) ls[i] = i;
	forn(i, n) {
		if (ls[dst[i]] != src[i]) return 0;
		x = ls[dst[i]];
		j = dst[i];
		while(j) (ls[j] = ls[j-1]), j--;
		ls[0] = x;
	}
	return 1;
}

void test_rle() {
	int i, j;
	uint bign = 1*1024*1024;
	uchar *big = randomCase(bign, 6);
	testAll("RLE/IRLE", (char**)tcases, sizeof(tcases)/sizeof(char*), rle_irle, (fvoid*)rle, (fvoid*)irle, 0);
	printf("big test: "); testCase(big, bign, rle_irle, (fvoid*)rle, (fvoid*)irle, 1, NULL);

	printf("RLE/IRLE random test, %u bytes\n[", bign);
	forn(i, 200) {
		forn(j, bign) big[j] = 'a' + rand()%5;
		testCase(big, bign, rle_irle, (fvoid*)rle, (fvoid*)irle, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_RLE end **/


/** TEST_BWTJ begin **/
typedef void(bwtj_run)(uchar*, uint, uchar*, uint, uint*, uint*, uint*);
typedef void(bwtj_run_prepared)(uint*, uint*, uint, uint, uint*);
typedef int(bwtj_test)(uchar*, uint, uchar*, uint, uint*); // 2 strings y la permutacion de salida

int check_bwtj(uchar* s, uint n, uchar* t, uint m, uint* idx) {
	uint i;
	uchar* mem = pz_malloc(2*(n+m+2));
	memcpy(mem, s, n);
	memcpy(mem+n+1, t, m);
	memcpy(mem+n+m+2, s, n);
	memcpy(mem+n+m+n+3, t, m);
	mem[n]=mem[n+m+1]=mem[n+m+n+2]=mem[n+m+n+m+3]=255;

	// Verifica que las strings estén en orden
	forn(i, n+m+2) {
		if (idx[i] < 0 || idx[i] >= n+m+2 ||
			(i > 0 && memcmp(mem+idx[i-1], mem+idx[i], n+m+2) > 0)) {
			//printf("TEST: "); forn(i,(n+m+2)) printf("%c", mem[i]); printf("\n");
			//forn(i,n+m+2) printf("%d ",idx[i]); printf("\n");
			pz_free(mem);
			//exit(0);
			return 0;
		}
	}
	pz_free(mem);
	return 1;
}

int bwtj_tester(uchar* st, uint nm, double* d, fvoid fr, fvoid ft) {
	uint n = rand()%(nm+1), m,i;
	forn(i,nm) {
		if (st[i]==255) {
			n = i;
			break;
		}
	}
	m = st[n]==255?nm-n-1:nm-n;
	forsn(i,nm-m,nm) if (st[i]==255) st[i]=254;
	uchar *s = st, *t = st+nm-m;
	//printf("s: "); forn(i,n) printf("%c",s[i]); printf("\tt: "); forn(i,m) printf("%c",t[i]); printf("\n");
	tiempo t1, t2;
	int res;
	uint *out = (uint*)pz_malloc((n+m+2) * sizeof(uint));
	uint *p = (uint*)pz_malloc((n+m+2) * sizeof(uint));
	getTickTime(&t1);
	(*(bwtj_run*)fr)(s, n, t, m, out, p, 0);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = (*(bwtj_test*)ft)(s, n, t, m, out);
	pz_free(out);
	pz_free(p);
	return res;
}


int bwtj_tester_prepared(uchar* st, uint nm, double* d, fvoid fr, fvoid ft) {
	uint n = rand()%(nm+1), m,i;
	forn(i,nm) {
		if (st[i]==255) {
			n = i;
			break;
		}
	}
	m = st[n]==255?nm-n-1:nm-n;
	forsn(i,nm-m,nm) if (st[i]==255) st[i]=254;
	uchar *s = st, *t = st+nm-m;
	tiempo t1, t2;
	int res;
	uint *out = (uint*)pz_malloc((n+m+2) * sizeof(uint));
	uint *p = (uint*)pz_malloc((n+m+2) * sizeof(uint));
	bwt_joint_prepare(s, n, t, m, out, p);
	getTickTime(&t1);
	(*(bwtj_run_prepared*)fr)(p, out, n, m, 0);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = (*(bwtj_test*)ft)(s, n, t, m, out);
	pz_free(out);
	pz_free(p);
	return res;
}

void test_bwtj() {
	uint bign = 512*1024;
	uchar *big = pz_malloc(bign);
	uint i, j, k, i2, n, m, al, sz;
	uint als[5] = {2,6,8,128,254};
	uint szs[10] = {30,50,70,90,100,150,300,512,1024,bign-5};

	testAll("BWTJ", (char**)tcases, sizeof(tcases)/sizeof(char*), bwtj_tester, (fvoid*)bwt_joint, (fvoid*)check_bwtj, 0);

	printf("Regression test\n[");
	char* stt = "aabbbbbbaxabbbb";
	uchar st[15];
	memcpy(st, stt, 15);
	st[9]=255;
	testCase((uchar*)st, 15, bwtj_tester, (fvoid*)bwt_joint, (fvoid*)check_bwtj, 0, NULL);

	printf("]\nBWTJ random test, %u bytes\n[", bign);
	forn(al,sizeof(als)/sizeof(uint))forn(sz,sizeof(szs)/sizeof(uint))forn(i, 5) { // Stress Test
		forn(j, bign) big[j] = 'a' + rand()%als[al];
		testCase(big, 1+rand()%szs[sz], bwtj_tester, (fvoid*)bwt_joint, (fvoid*)check_bwtj, 0, NULL);
	}

	printf("]\nBWTJ random test, 2^n-1, 2^n and 2^n+1 bytes\n[");
	forsn(i, 1, 15) { // 2^n+-1 length
		forn(k, 10) {
			forn(j, ((1<<i)+1)) big[j] = 'a' + rand()%6;
			testCase(big, (1<<i)-1, bwtj_tester, (fvoid*)bwt_joint, (fvoid*)check_bwtj, 0, NULL);
			testCase(big, (1<<i), bwtj_tester, (fvoid*)bwt_joint, (fvoid*)check_bwtj, 0, NULL);
			testCase(big, (1<<i)+1, bwtj_tester, (fvoid*)bwt_joint, (fvoid*)check_bwtj, 0, NULL);
		}
	}
	printf("]\nBWTJ random test, 2^n-1, 2^n and 2^n+1 vs 2^m-1, 2^m and 2^m+1 bytes\n[");
	forsn(i, 5, 11)forsn(i2, 5, 11)forsn(n,(1<<i)-1,(1<<i)+2)forsn(m,(1<<i2)-1,(1<<i2)+2) {
		forn(k, 2) {
			forn(j, (n+m+1)) big[j] = 'a' + rand()%6;
			big[n]=255;
			testCase(big, n+m+1, bwtj_tester, (fvoid*)bwt_joint, (fvoid*)check_bwtj, 0, NULL);
		}
	}
	printf("]\n");
	pz_free(big);

	testAll("BWTJ prepared", (char**)tcases, sizeof(tcases)/sizeof(char*), bwtj_tester_prepared, (fvoid*)bwt_joint_hint, (fvoid*)check_bwtj, 0);
}

void test_bwt_cmp_size(uint bign) {
	uint i,j,k,n,c=30;
	uchar* big=(uchar*)pz_malloc(bign);
	double d,dj,avg=0.0;
	printf("BWTJ vs BWT random test, %u bytes\n", bign);
	forn(i, c) { // Stress Test
		forn(j, bign) big[j] = 'a' + rand()%8;
		k = bign/2+(rand()%(bign/2+1)); /*Only test on really big inputs*/
		n = k/3+rand()%(k/3); /*Partition in similar sizes*/
		big[n] = 255;
		printf("HINT   test %u/%u: ", i+1, c);
		testCase(big, k, bwtj_tester_prepared, (fvoid*)bwt_joint_hint, (fvoid*)check_bwtj, 1, &dj);
		printf("NORMAL test %u/%u: ", i+1, c);
		testCase(big, k, bwt_tester, (fvoid*)bwt, (fvoid*)check_bwt, 1, &d);
		avg+=dj/d;
		printf("*** SIZE=%u HINT/NORMAL: %.4f AVG=%.3f***\n", k, dj/d, avg/(i+1));
	}
	pz_free(big);
}

void test_bwt_cmp() {
	test_bwt_cmp_size(2*1024*1024);
}

void test_bwt_cmp_big() {
	test_bwt_cmp_size(20*1024*1024);
}

void test_bwt_cmp_huge() {
	test_bwt_cmp_size(200*1024*1024);
}
/** TEST_BWTJ end **/

/** TEST_LZW begin **/
typedef uint(lzw_run)(uchar*, ushort*, uint);
typedef uint(lzw_test)(ushort*, uchar*, uint);

int lzw_ilzw(uchar* src, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	int res;
	uint lzwz, ilzwz;
	ushort *out = pz_malloc(n*sizeof(ushort));
	uchar *src2 = pz_malloc(n);
	getTickTime(&t1);
	lzwz = (*(lzw_run*)fr)(src, out, n);
	ilzwz = (*(lzw_test*)ft)(out, src2, lzwz);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = (ilzwz == n) && (!memcmp(src, src2, n));
	pz_free(out);
	pz_free(src2);
	return res;
}

void test_lzw() {
	int i, j, k;
	uint bign = 512*1024;
	uchar *big = randomCase(bign, 6);

	testAll("LZW/ILZW", (char**)tcases, sizeof(tcases)/sizeof(char*), lzw_ilzw, (fvoid*)lzw, (fvoid*)ilzw, 0);
	printf("big test: "); testCase(big, bign, lzw_ilzw, (fvoid*)lzw, (fvoid*)ilzw, 1, NULL);

	uchar cs[] = "bacbacbacbacbac";
	printf("reg: "); testCase(cs, strlen((char*)cs), lzw_ilzw, (fvoid*)lzw, (fvoid*)ilzw, 1, NULL);

	printf("LZW/ILZW random test, %u bytes\n[", bign);
	forn(i, 398) {
		k = 2 + rand()%8;
		forn(j, bign) big[j] = 'a' + rand()%k;
		testCase(big, bign, lzw_ilzw, (fvoid*)lzw, (fvoid*)ilzw, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_LZW end **/

/** TEST_HUFFMAN begin **/
void test_huf() {
	int i, j, k, l, p;
	uint bign = 512*1024;
	uint64 res;
	uint tres;
	ushort *big = (ushort*)pz_malloc(bign*sizeof(ushort));

	forn(i, 1) {
		k = 19;
//		forn(j, bign) big[j] = 'A' + rand()%k + rand()%k + rand()%k;
		p = 0;
		forn(j, k) {
			l = 1 << j; while(l--) big[p++] = 'A' + j;
		}
		res = huffman_compress_len(big, p, 110, &tres);
		printf("res = %llu\n", (res+7)/8);
		printf("tres = %u\n", (tres+7)/8);

	}

	pz_free(big);
}
/** TEST_HUFFMAN end **/


/** TEST_LCP begin **/
typedef int(lcp_run)(uint, uchar*, uint*, uint*);
typedef int(lcp_test)(uint, uchar*, uint*, uint*);

int check_lcp(uint n, uchar* src, uint* r, uint* h) {
	uint i;
	uchar* s2 = (uchar*)pz_malloc(2 * n * sizeof(uchar));
	memcpy(s2, src, n);
	memcpy(s2+n, src, n);
	forn(i,n-1) {
		if (memcmp(s2+r[i],s2+r[i+1],h[i]) != 0) return 0;
		if (h[i] < n && s2[r[i]+h[i]] == s2[r[i+1]+h[i]]) return 0;
	}
	return 1;
	pz_free(s2);
}

int lcp_tester(uchar* osrc, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	uint i;
	uint *p, *r, *rp;
	uchar* src;
	int res;
	src = (uchar*)pz_malloc(n * sizeof(uchar));
	memcpy(src, osrc, n);
	src[n-1] = 255; //avoid rotations
	p = (uint*)pz_malloc(n * sizeof(uint));
	r = (uint*)pz_malloc(n * sizeof(uint));
	rp = (uint*)pz_malloc(n * sizeof(uint));
	memcpy(p, src, n);
	bwt(NULL, p, r, NULL, n, 0);
	forn(i,n) p[r[i]] = i;
	getTickTime(&t1);
	memcpy(rp, r, n * sizeof(uint));
	((lcp_run*)fr)(n, src, r, p);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = (*(lcp_test*)ft)(n, src, rp, r);
	if (!res) {
		printf("failed testing "); forn(i,n) printf("%c",(char)src[i]);printf("\n");
		printf ("with r = {"); forn(i,n) printf("%d,",r[i]); printf("}\n");
		printf (" and p = {"); forn(i,n) printf("%d,",p[i]); printf("}\n");
		exit(1);
	}
	pz_free(r);
	pz_free(rp);
	pz_free(p);
	pz_free(src);
	return res;
}

void test_lcp() {
	int i, j;
	uint bign = 1*1024*1024, rndn = 128*1024;
	uchar *big = randomCase(bign, 6);
	testAll("LCP", (char**)tcases, sizeof(tcases)/sizeof(char*), lcp_tester, (fvoid*)lcp, (fvoid*)check_lcp, 0);
	printf("big test: "); testCase(big, bign, lcp_tester, (fvoid*)lcp, (fvoid*)check_lcp, 1, NULL);

	printf("LCP random test, %u bytes\n[", rndn);
	forn(i, 200) {
		uint sz = rand()%(rndn/2) + rndn/2;
		forn(j, sz) big[j] = 'a' + rand()%5;
		testCase(big, sz, lcp_tester, (fvoid*)lcp, (fvoid*)check_lcp, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_LCP end **/

/** TEST_KINVLCP begin **/
typedef long double(klcp_run)(uint, uint*);
typedef long double(klcp_test)(uint, uint*);

int klcp_tester(uchar* osrc, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	uint i;
	uint *p, *r, *h;
	uchar* src;
	long double res, res2;
	src = (uchar*)pz_malloc(n * sizeof(uchar));
	memcpy(src, osrc, n);
	src[n-1] = 255; //avoid rotations
	p = (uint*)pz_malloc(n * sizeof(uint));
	r = (uint*)pz_malloc(n * sizeof(uint));
	h = (uint*)pz_malloc(n * sizeof(uint));
	memcpy(p, src, n);
	bwt(NULL, p, r, NULL, n, 0);
	forn(i,n) p[r[i]] = i;
	lcp(n, src, r, p);
	memcpy(h, r, n * sizeof(uint));
	getTickTime(&t1);
	res = (*(klcp_run*)fr)(n, h);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res2 = (*(klcp_run*)ft)(n, r);
	if (fabs(res-res2)>1e-10 && fabs(res-res2)/(1.0+res2)>1e-10) {
		printf("failed testing ");
		if (n <1000) {
			forn(i,n) printf("%c",(char)src[i]);printf("\n");
			printf("with h = {"); forn(i,n) printf("%d,",r[i]); printf("}\n");
		}
		printf("with res = %lf and %lf (dif: %.20lf)\n", (double)res, (double)res2, (double)(res-res2));
		exit(1);
	}
	pz_free(r);
	pz_free(h);
	pz_free(p);
	pz_free(src);
	return res;
}

void test_klcp() {
	int i, j;
	uint bign = 1*1024*1024, rndn = 128*1024;
	uchar *big = randomCase(bign, 6);
	testAll("KINVLCP", (char**)tcases, sizeof(tcases)/sizeof(char*), klcp_tester, (fvoid*)klcp, (fvoid*)klcp_lin, 0);
	printf("big test: "); testCase(big, bign, klcp_tester, (fvoid*)klcp, (fvoid*)klcp_lin, 1, NULL);

	printf("KINVLCP random test, %u bytes\n[", rndn);
	forn(i, 200) {
		uint sz = rand()%(rndn/2) + rndn/2;
		forn(j, sz) big[j] = 'a' + rand()%5;
		testCase(big, sz, klcp_tester, (fvoid*)klcp, (fvoid*)klcp_lin, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_KINVLCP end **/

/** TEST_BIT begin **/
void printbin(uint c) {
	uint i;
	char buf[33] = "00000000";
	forn(i,32) {
		buf[i] = '0' + (c & 1);
		c >>= 1;
	}
	printf("%s", buf);
}

int bit_tester(uchar* src, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	uint i;
	int res;
	getTickTime(&t1);
	bitarray* bitarray1 = bita_malloc(n*8);
	bitarray* bitarray2 = bita_malloc(n*8);
	forn(i, n) ((uchar*)bitarray1)[i] = rand();
	forn(i, n) ((uchar*)bitarray2)[i] = rand();
	bitarray1[0]=0;
	forn(i,n*8) {
		if (!((src[i/8] >> (i%8)) & 1)) {
			bita_set(bitarray1, i);
		} else {
			bita_unset(bitarray1, i);
		}
	}
	dforn(i,n*8) {
		if (!bita_get(bitarray1, i)) {
			bita_set(bitarray2, i);
		} else {
			bita_unset(bitarray2, i);
		}
	}
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	res = 1;
	forn(i,n*8) {
		if (((src[i/8] >> (i%8)) & 1) != bita_get(bitarray2, i)) {
			res = 0;
		}
	}
	pz_free(bitarray1);
	pz_free(bitarray2);
	return res;
}

void test_bit() {
	int i, j, k;
	uint bign = 512*1024;
	uchar *big = randomCase(bign, 6);

	testAll("BIT", (char**)tcases, sizeof(tcases)/sizeof(char*), bit_tester, (fvoid*)NULL, (fvoid*)NULL, 0);
	printf("big test: "); testCase(big, bign, bit_tester, (fvoid*)NULL, (fvoid*)NULL, 1, NULL);

	printf("BIT random test, %u bytes\n[", bign);
	forn(i, 70) {
		k = 2 + rand()%8;
		forn(j, bign) big[j] = 'a' + rand()%k;
		testCase(big, bign, bit_tester, (fvoid*)NULL, (fvoid*)NULL, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_BIT end **/

/** TEST_TREE begin **/
int tree_tester(uchar* src, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	uint i,j,k,aux;
	int res;
	uint* arr = (uint*)pz_malloc(n * sizeof(uint));
	uint* us = bita_malloc(n+2);
	forn(i,n+2) bita_unset(us, i);
	bittree* tree = bittree_malloc(n+2);
	bittree_clear(tree, n+2);
	bittree_set(tree, n+2, 0);
	bittree_set(tree, n+2, n+1);
	forn(i,n) arr[i]=i+1;
	forn(i,10*n) {
		j = rand()%n;
		k = rand()%n;
		aux = arr[k]; arr[k]=arr[j]; arr[j] = aux;
	}
	bita_set(us, 0);
	bita_set(us, n+1);
	*d = 0;
	res = 1;
	forn(i,n) {
		getTickTime(&t1);
		uint mlt = bittree_max_less_than(tree, n+2, arr[i]);
		uint mgt = bittree_min_greater_than(tree, n+2, arr[i]);
		bittree_set(tree, n+2, arr[i]);
		getTickTime(&t2);
		*d += getTimeDiff(t1, t2);
		if (!bita_get(us, mlt) || !bita_get(us, mgt)) {
			res = 0;
			break;
		}
		forsn(j,mlt+1,mgt) if (bita_get(us,j)) {
			res = 0;
			break;
		}
		if (!res) break;
		bita_set(us, arr[i]);
	}
	pz_free(us);
	pz_free(arr);
	bittree_free(tree, n+2);
	if (!res) exit(1);
	return res;
}

void test_tree() {
	int i, j;
	uint bign = 1024;

	printf("TREE random test, %u bytes\n[", bign);
	forn(j,3)forn(i, 70) {
		testCase(NULL, i, tree_tester, (fvoid*)NULL, (fvoid*)NULL, 0, NULL);
	}
	printf("]\n");
	printf("big test: "); testCase(0, bign, tree_tester, (fvoid*)NULL, (fvoid*)NULL, 1, NULL);
}
/** TEST_TREE end **/

/** TEST_RMQ begin **/
int rmq_tester(uchar* src, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	uint i, a, b, j, x, vl, vlb;
	int res;
	uint* buf = (uint*)pz_malloc(sizeof(uint)*n);
	forn(i, n) buf[i] = n+200+src[i];
	getTickTime(&t1);
	rmq *q = rmq_malloc(n);
	rmqbit *qb = rmqbit_malloc(n);
	rmq_init_min(q, buf, n);
	rmqbit_init_min(qb, buf, n);
	res = 1;

	dforn(i, n) {
		buf[i] = i+100;
		rmq_update_min(q, i, n);
		rmqbit_update_min(qb, i, n);
	}

	forn(i, 4096) {
		a = rand()%(n/2+1);
		b = a + rand()%(n/2+1) + 1;
		if (b>n) b = n;
		if (a>=b) a = b-1;

		vl = rmq_query_min(q, a, b);
		vlb = rmqbit_query_min(qb, a, b);
		x = buf[a]; forsn(j, a, b) if (x > buf[j]) x = buf[j];

		if (vl != x) { fprintf(stderr, "RMQ error:\n"); DBGu(vl); DBGu(x); DBGu(a); DBGu(b); res = 0; break; }
		if (vlb != x) {
			fprintf(stderr, "RMQ-bit error:\n"); DBGu(vlb); DBGu(x); DBGu(a); DBGu(b); res = 0;
			forsn(j, a, b) fprintf(stderr, "%u ", buf[j]); fprintf(stderr, "\n");
			rmqbit_show(qb, n); rmqbit_bitshow(qb, n); rmqbit_init_min(qb, buf, n); rmqbit_show(qb, n);
			exit(0); break; }

		j = (a+b)/2;
		buf[j] = j;
		rmq_update_min(q, j, n);
		rmqbit_update_min(qb, j, n);

	}

	rmq_free(q, n);
	rmqbit_free(qb, n);

	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);

	pz_free(buf);

	return res;
}

void test_rmq() {
	int i, j, k;
	uint bign = 16*1024;
	uchar *big = randomCase(bign, 6);

	testAll("RMQ", (char**)tcases, sizeof(tcases)/sizeof(char*), rmq_tester, (fvoid*)NULL, (fvoid*)NULL, 0);
	printf("big test: "); testCase(big, bign, rmq_tester, (fvoid*)NULL, (fvoid*)NULL, 1, NULL);

	printf("RMQ random test, %u bytes\n[", bign);
	forn(i, 70) {
		k = 2 + rand()%8;
		forn(j, bign) big[j] = 'a' + rand()%k;
		testCase(big, bign-rand()%1024, rmq_tester, (fvoid*)NULL, (fvoid*)NULL, 0, NULL);
		testCase(big, 1 + rand()%bign, rmq_tester, (fvoid*)NULL, (fvoid*)NULL, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_RMQ end **/

/** TEST_MRS begin **/
typedef int(mrs_run)(uchar* s, uint n, uint* r, uint* h, uint* p, uint ml,
					 output_callback out, void* data);
typedef int(mrs_test)(uchar* s, uint n, uint* r, uint ml, FILE* in);

int check_mrs(uchar* s, uint n, uint* r, uint ml, FILE* in) {

	return 1;
}

int mrs_tester(uchar* osrc, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	int res;
	uint *r, *h, *p;
	uint ml = 1, i;
	uchar* src;
	FILE* out = fopen("__test__", "wt");
	if (!out) return 0;
	src = (uchar*)pz_malloc(n * sizeof(uchar));
	r = (uint*)pz_malloc(n * sizeof(uint));
	h = (uint*)pz_malloc((n-1) * sizeof(uint));
	p = (uint*)pz_malloc(n * sizeof(uint));
	memcpy(src, osrc, n * sizeof(uchar));
	src[n-1] = 255;
	bwt(NULL, p, r, src, n, NULL);
	forn(i,n) p[r[i]] = i;
	lcp(n, src, r, p);
	memcpy(h, r, (n-1) * sizeof(uint));
	forn(i,n) r[p[i]] = i;
	/*printf("s = "); forn(i,n) printf("%c", src[i]); printf(" (%d) \n", n);
	printf(".    "); forn(i,n) printf("%d", i%10); printf(" (%d) \n", n);
	printf("r ="); forn(i,n) printf(" %d", r[i]); printf("\n");
	printf("p ="); forn(i,n) printf(" %d", p[i]); printf("\n");
	printf("h ="); forn(i,n-1) printf(" %d", h[i]); printf("\n");*/
	getTickTime(&t1);
	(*(mrs_run*)fr)(src, n, r, h, p, ml, output_file_text, out);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);
	fclose(out);
	out = fopen("__test__", "rt");
	if (!out) {
		res = 0;
	} else {
		res = (*(mrs_test*)ft)(src, n, r, ml, out);
	}
	pz_free(src);
	pz_free(r);
	pz_free(h);
	pz_free(p);
	return res;
}

void test_mrs() {
	int i, j;
	uint bign = 1*1024*1024, rndn = 128*1024;
	uchar *big = randomCase(bign, 6);
	testAll("MRS", (char**)tcases, sizeof(tcases)/sizeof(char*), mrs_tester, (fvoid*)mrs, (fvoid*)check_mrs, 0);
	printf("big test: "); testCase(big, bign, mrs_tester, (fvoid*)mrs, (fvoid*)check_mrs, 1, NULL);

	printf("MRS random test, %u bytes\n[", rndn);
	forn(i, 200) {
		uint sz = rand()%(rndn/2) + rndn/2;
		forn(j, sz) big[j] = 'a' + rand()%5;
		testCase(big, sz, mrs_tester, (fvoid*)mrs, (fvoid*)check_mrs, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_MRS end **/

/** TEST_LONGPAT begin **/
typedef struct {
	uint n, res;
	uchar* s;
	uint *r;
	uint *pat_l, *pat_d;
	uint *buf;
} lp_data;

void lp_data_init(lp_data* this, uint n, uint *r, uchar* s) {
	this->n = n;
	this->r = r;
	this->s = s;
	this->pat_l = (uint*)pz_malloc(n * sizeof(uint));
	this->pat_d = (uint*)pz_malloc(n * sizeof(uint));
	this->buf = (uint*)pz_malloc(n * sizeof(uint));
	memset(this->pat_l, 0x00, n * sizeof(uint));
	memset(this->pat_d, 0x00, n * sizeof(uint));
	memset(this->buf,   0x00, n * sizeof(uint));
	this->res = 1;
}

void lp_data_destroy(lp_data* this) {
	pz_free(this->pat_l);
	pz_free(this->pat_d);
	pz_free(this->buf);
}

void show_bwt_lcp(uint n, uchar* src, uint* r, uint* h) {
	int i, j;
	printf("\n");
	printf(" i  r[] lcp \n");
	forn(i, n) {
		printf("%3d %3d %3d ", i, r[i], h[i]);
		forn(j, n) printf("%c", src[(r[i]+j)%n]);
		printf("\n");
	}
}

#define ID(X) (*(X))
_def_qsort(sort_uint, uint, uint, ID, <)
void longpat_mrs_cb(uint l, uint i, uint c, lp_data* this) {
	uint j;
	forn(j, c) this->buf[j] = this->r[i+j];
	sort_uint(this->buf, this->buf+c);
	forsn(j, 1, c) {
		this->pat_l[this->buf[j]] = l;
		this->pat_d[this->buf[j]] = this->buf[j] - this->buf[j-1];
	}
}

void longpat_cb(uint i, uint len, uint delta, lp_data* this) {
	if ((this->pat_l[i] != len) || (this->pat_d[i] != delta)) {
		fprintf(stderr, "at i=%u, (%u, %u) [mrs] != (%u, %u) [longpat]\n", i, this->pat_l[i], this->pat_d[i], len, delta);
		DBGcn(this->s+i-this->pat_d[i], this->pat_l[i]);
		DBGcn(this->s+i, this->pat_l[i]);
		DBGcn(this->s+i-delta, len);
		DBGcn(this->s+i, len);
		this->res = 0;
	}
}

int longpat_tester(uchar* osrc, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	uint *r, *h, *p;
	uint ml = 1;
	uchar* src, *bwto;
	lp_data dt;
	bittree* bwtt;
	n++;
	src = (uchar*)pz_malloc(n * sizeof(uchar));
	bwto = (uchar*)pz_malloc(n * sizeof(uchar));
	r = (uint*)pz_malloc(n * sizeof(uint));
	h = (uint*)pz_malloc(n * sizeof(uint));
	p = (uint*)pz_malloc(n * sizeof(uint));
	memcpy(src, osrc, (n-1) * sizeof(uchar));
	src[n-1] = 255;
	bwt(bwto, p, r, src, n, NULL);
//	forn(i,n) p[r[i]] = i;
	memcpy(h, r, n * sizeof(uint));
	lcp(n, src, h, p);
	lp_data_init(&dt, n, r, src);
	mrs(src, n, r, h, p, ml, (output_callback*)longpat_mrs_cb, &dt);

	getTickTime(&t1);
	bwtt = longpat_bwttree(bwto, n);
	longpat(src, n, r, h, p, bwtt, (longpat_callback*)longpat_cb, &dt);
	longpat_bwttree_free(bwtt, n);
	getTickTime(&t2);
	*d = getTimeDiff(t1, t2);

	if (!dt.res) {
		fflush(stdout);
		DBGcn(src, n-1);
		show_bwt_lcp(n, src, r, h);
	}

	lp_data_destroy(&dt);
	pz_free(src);
	pz_free(bwto);
	pz_free(r);
	pz_free(h);
	pz_free(p);
	if (!dt.res) {
		exit(1);
	}
	return dt.res;
}

void test_longpat() {
	int i, j;
	uint bign = 1*1024*1024, rndn = 128*1024;
	uchar *big = randomCase(bign, 6);
	testAll("LONGPAT", (char**)tcases, sizeof(tcases)/sizeof(char*), longpat_tester, NULL, NULL, 0);
	printf("LONGPAT random test, %u bytes\n[", rndn);
	forn(i, 200) {
//		rndn = (i+1)*bign / (201);
		uint sz = rand()%(rndn/2) + rndn/2;
		forn(j, sz) big[j] = 'a' + rand()%7;
		testCase(big, sz, longpat_tester, NULL, NULL, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_LONGPAT end **/

/** TEST_PERF begin **/
#define perf_passes 8
int perf_cache(uint n, uint r) {
	tiempo t[perf_passes+1];
	uint i, j, p, m, *buf;
	double d[perf_passes], tot;

	register uint a = 0;

	m = n * (1024 / sizeof(uint));
	buf = (uint*)pz_malloc(m*sizeof(uint));
	memset(buf, 0, m*sizeof(uint));
	forn(i, m) buf[i] = i*i%1003;

	forn(p, perf_passes) {
		getTickTime(t+p);
		forn(j, r) forn(i, m) a += buf[i];
	}
	getTickTime(t+perf_passes);
	if (a==-1) printf("a=0!\n");

	forn(p, perf_passes) d[p] = getTimeDiff(t[p], t[p+1]);

	tot = d[0];
	forn(p, perf_passes) if (tot > d[p]) tot = d[p];

	printf("%7uKB ", n);
	forn(p, perf_passes) printf("%8.3f ", d[p]);
	printf(" | %8.3f us/KB | %9.3f ms/pass (%u)\n", tot*1000.0/n/r, tot/r, r);


	pz_free(buf);
	return 1;
}


void test_perf() {
	uint i;
	double n = 2.0;

	forn(i, 37) {
		perf_cache((int)(n), 10000/n+1);
		n *= 1.5;
	}
}
/** TEST_PERF end **/

/** TEST_BITSTREAM begin **/
int bitstream_tester(uchar* _src, uint _n, double* _d, fvoid _fr, fvoid _ft) {
#define CANT 4096
	uint *cant_bits = (uint *) pz_malloc(sizeof(uint) * CANT);
	uint *bits = (uint *) pz_malloc(sizeof(uint) * CANT);
	obitstream *o = (obitstream *) pz_malloc(sizeof(obitstream));
	ibitstream *i = (ibitstream *) pz_malloc(sizeof(ibitstream));
	FILE *f;
	int j, k;

	/* genero CANT pedacitos al azar. el i-esimo pedacito ocupa cant_bits[i] bits
	* y su valor es bits[i] */
	uint bits_totales = 0;
	forn(j, CANT) {
		cant_bits[j] = (uint)(((float)rand() / (float)RAND_MAX) * 64 + 1);
		bits_totales += cant_bits[j];
		bits[j] = rand() & ((1 << cant_bits[j]) - 1);
	}

	uint bytes_totales = (bits_totales + 7) / 8;

#define Assert(X) \
	if (!(X)) {\
		fprintf(stderr, "bitstream_tester - assertion failed: " #X "\n");\
		pz_free(cant_bits);\
		pz_free(bits);\
		pz_free(o);\
		pz_free(i);\
		if (f) fclose(f);\
		return 0;\
	}

	/* escribo todo dos veces usando dos obitstreams en el mismo archivo */
	/* test write_uint */
	f = fopen("__test__", "w");
	obs_create(o, f);
	forn (j, CANT) {
		obs_write_uint(o, cant_bits[j], bits[j]);
	}
	obs_destroy(o);

	obs_create(o, f);
	forn (j, CANT) {
		obs_write_uint(o, cant_bits[j], bits[j]);
	}
	obs_destroy(o);

	/* test write_bit */
	obs_create(o, f);
	forn (j, CANT) {
		int num = bits[j];
		forn (k, 32) {
			obs_write_bit(o, num & 1);
			num >>= 1;
		}
	}
	obs_destroy(o);

	/* test write_bbpb */
	obs_create(o, f);
	forn (j, 100)
		obs_write_bbpb(o, j);
	obs_destroy(o);

	obs_create(o, f);
	forn (j, 800)
		obs_write_bbpb(o, 0);
	forn (j, 400)
		obs_write_bbpb(o, 1);
	obs_destroy(o);

	obs_create(o, f);
	forn (j, 100)
		obs_write_bbpb(o, 99 - j);
	obs_destroy(o);

	fclose(f);

	f = NULL;
	Assert(filesize("__test__") == 2 * bytes_totales + 4 * CANT + 144 + 100 + 100 + 144);

	/* leo todo dos veces usando dos ibitstreams en el mismo archivo */

	/* test read_uint */
	uint cant_leidos;
	f = fopen("__test__", "r");
	ibs_create(i, f, bytes_totales);
	cant_leidos = 0;
	forn (j, CANT) {
		cant_leidos += cant_bits[j];
		uint leido = ibs_read_uint(i, cant_bits[j]);
		Assert(cant_leidos <= bits_totales);
		Assert(bits[j] == leido);
	}
	ibs_destroy(i);

	ibs_create(i, f, bytes_totales);
	cant_leidos = 0;
	forn (j, CANT) {
		cant_leidos += cant_bits[j];
		uint leido = ibs_read_uint(i, cant_bits[j]);
		Assert(cant_leidos <= bits_totales);
		Assert(bits[j] == leido);
	}
	ibs_destroy(i);

	/* test read_bit */
	ibs_create(i, f, 4 * CANT);
	forn (j, CANT) {
		int num = bits[j];
		forn (k, 32) {
			Assert(ibs_read_bit(i) == (num & 1));
			num >>= 1;
		}
	}
	
	ibs_destroy(i);

	/* test read_bbpb */
	ibs_create(i, f, 144);
	forn (j, 100)
		Assert(ibs_read_bbpb(i) == j);
	ibs_destroy(i);

	ibs_create(i, f, 100 + 100);
	forn (j, 800)
		Assert(ibs_read_bbpb(i) == 0);
	forn (j, 400) {
		Assert(ibs_read_bbpb(i) == 1);
	}
	ibs_destroy(i);

	ibs_create(i, f, 144);
	forn (j, 100)
		Assert(ibs_read_bbpb(i) == 99 - j);
	ibs_destroy(i);

	fclose(f);
	pz_free(o);
	pz_free(i);
	pz_free(cant_bits);
	pz_free(bits);
#undef Assert
#undef CANT
	return 1;
}

void test_bitstream() {
	int i;
	srand(time(NULL));
	printf("BITSTREAM random test\n[");
	forn(i, 200) {
		testCase(NULL, 0, bitstream_tester, NULL, NULL, 0, NULL);
	}
	printf("]\n");
}
/** TEST_BITSTREAM end **/

/** TEST_SASEARCH begin **/
int sasearch_tester(uchar* osrc, uint n, double* d, fvoid fr, fvoid ft) {
	tiempo t1, t2;
	int res;
	uint *r, *h, *p, *llcp, *rlcp;
	uint i, its, len, j;
	uchar* src, *pat;
	uint a_from, b_from, a_to, b_to, a_res, b_res;
	double tm = 0;
	src = (uchar*)pz_malloc(n * sizeof(uchar));
	pat = (uchar*)pz_malloc(n * sizeof(uchar));
	r = (uint*)pz_malloc(n * sizeof(uint));
	h = (uint*)pz_malloc(n * sizeof(uint));
	llcp = (uint*)pz_malloc(n * sizeof(uint));
	rlcp = (uint*)pz_malloc(n * sizeof(uint));
	p = (uint*)pz_malloc(n * sizeof(uint));
	memcpy(src, osrc, n * sizeof(uchar));
	src[n-1] = 255;
	bwt(NULL, p, r, src, n, NULL);
	forn(i,n) p[r[i]] = i;
	lcp(n, src, r, p);
	memcpy(h, r, (n-1) * sizeof(uint));
	forn(i,n) r[p[i]] = i;
	
	build_lrlcp(n, h, llcp, rlcp);
	res = 1;
	forn(its, 30) {
		len = 30; if (n/2 < len) len = n/2;
		if (its < 5) forn(j, len) pat[j] = src[rand()%(n-1)];
		else memcpy(pat, src + rand()%(n-len), len);
		
		a_res = sa_search(src, r, n, pat, len, &a_from, &a_to);
		getTickTime(&t1);
		b_res = sa_mm_search(src, r, n, pat, len, &b_from, &b_to, llcp, rlcp);
		getTickTime(&t2);
		tm += getTimeDiff(t1, t2);
		res = res && (a_res == b_res) && (((a_to == b_to) && (a_from == b_from)));
		if (!res) {
			fprintf(stderr, "\nsrc = «%s»\npat = «%s»\nsasearch = %d [%d,%d), mmsearch = %d [%d,%d)\n", src, pat, a_res, a_from, a_to, b_res, b_from, b_to);
			exit(0);
		}
	}
	*d = tm;
	
	pz_free(src);
	pz_free(pat);
	pz_free(r);
	pz_free(h);
	pz_free(llcp);
	pz_free(rlcp);
	pz_free(p);
	return res;
}

void test_sasearch() {
	int i, j;
	uint bign = 1*1024*1024, rndn = 128*1024;
	uchar *big = randomCase(bign, 6);
	testAll("SASEARCH", (char**)tcases, sizeof(tcases)/sizeof(char*), sasearch_tester, NULL, NULL, 0);
	printf("big test: "); testCase(big, bign, sasearch_tester, NULL, NULL, 1, NULL);

	printf("SASEARCH random test, %u bytes\n[", rndn);
	forn(i, 200) {
		uint sz = rand()%(rndn/2) + rndn/2;
		forn(j, sz) big[j] = 'a' + rand()%5;
		testCase(big, sz, sasearch_tester, NULL, NULL, 0, NULL);
	}
	printf("]\n");

	pz_free(big);
}
/** TEST_SASEARCH end **/


void test_all() {
	/*Only runs correctness test, not performance tests*/
	test_mtf();
	test_rle();
	test_sort();
	test_bwt();
	test_obwt();
	test_bwtj();
	test_lzw();
	test_huf();
	test_lcp();
	test_klcp();
	test_bit();
	test_tree();
	test_mrs();
	test_bitstream();
}

int main(int argc, char* argv[]) {
	int sed = 314159; /* Semilla */
	int i;
	printf("p2zip - Unit test - rev%d\n", SVNREVISION);

	if (argc <= 1) {
		fprintf(stderr, "Usage: %s [-s<randseed>] [-u] [-p] units [unit2 unit3 ...]\n", argv[0]);
		fprintf(stderr, "  -s<randseed>  Uses <randseed> for random seed. Default %d\n", sed);
		fprintf(stderr, "  units         Run tests for that unit. \n"
		                "                 Valid units: mtf, rle, sort, bwt, obwt, bwtj, bwt_cmp, bwt_cmp_big,\n"
		                "                 bwt_cmp_huge, obwt_cmp, obwt_cmp_big, obwt_cmp_huge, lzw, huf, lcp,\n"
		                "                 bit, tree, bitstream, sasearch, perf, all\n");
		return 1;
	}
#define TEST_UNIT(X) if (!strcmp(argv[i], #X)) test_##X();
	forsn(i, 1, argc) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 's') sed = atoi(argv[i]+2);
			else {
				fprintf(stderr, "%s: Unknown option '%s'\n", argv[0], argv[i]);
			}
		} else {
			srand(sed);
			TEST_UNIT(bwt) else
			TEST_UNIT(obwt) else
			TEST_UNIT(mtf) else
			TEST_UNIT(sort) else
			TEST_UNIT(rle) else
			TEST_UNIT(bwtj) else
			TEST_UNIT(bwt_cmp) else
			TEST_UNIT(bwt_cmp_big) else
			TEST_UNIT(bwt_cmp_huge) else
			TEST_UNIT(obwt_cmp) else
			TEST_UNIT(obwt_cmp_big) else
			TEST_UNIT(obwt_cmp_huge) else
			TEST_UNIT(lzw) else
			TEST_UNIT(huf) else
			TEST_UNIT(lcp) else
			TEST_UNIT(bit) else
			TEST_UNIT(tree) else
			TEST_UNIT(rmq) else
			TEST_UNIT(mrs) else
			TEST_UNIT(longpat) else
			TEST_UNIT(bitstream) else
			TEST_UNIT(perf) else
			TEST_UNIT(klcp) else
			TEST_UNIT(sasearch) else
			TEST_UNIT(all) else
			{
				fprintf(stderr, "%s: Unknown unit '%s'\n", argv[0], argv[i]);
			}
		}

	}


	return 0;
}
