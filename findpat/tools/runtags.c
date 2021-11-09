#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "lcp.h"
#include "cop.h"
#include "tipos.h"
#include "common.h"
#include "macros.h"
#include "output_callbacks.h"

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

int main(int argc, char** argv) {
	uint *p, *r, *h, *m, *mc, *nc, *tn;
	uchar *s, *st;
	uchar **t;
	uint sn,n,i,j,k,wrt, all = 0, allm = 0, tag = 1;
	int ps = -1, at = -1, bt = 0;

	wrt = 0;
	forsn(i, 1, argc) {
		if (0) {}
		else cmdline_opt_1(i, "-wrt") { wrt = 1; }
		else cmdline_opt_1(i, "-all") { all = 1; }
		else cmdline_opt_1(i, "-min") { allm = 1; }
		else {
			if (ps == -1) ps = i;
			if (wrt) bt++; // filter strings
			else at++;	// set strings
		}
	}

//	printf("base = %s, set = %d filter = %d\n", argv[1], at, bt); //DEBUG

	if (at < 0 || bt <= 0 || (all && allm)) {
		fprintf(stderr, "Usage: %s <string_1> <string_2> ... <string_n>"
						" -wrt <filter_1> <filter_2> ... <filter_m>"
						"[-all | -min]\n"
						"  -all shows all tags of minimum length\n"
						"  -min shows all minimal tags\n"
						, argv[0]);
		return 1;
	}

	s = (uchar*)argv[ps]; // base string
	sn = strlen(argv[ps]) + 1;
	s[sn-1] = 255;

	m = (uint*)pz_malloc(sn*sizeof(uint));

	mc = (uint*)pz_malloc(sn*sizeof(uint)); // array M (keep min)
	nc = (uint*)pz_malloc(sn*sizeof(uint)); // array N (keep max)
	forn(i,sn) mc[i] = sn;
	forn(i,sn) nc[i] = 0;


	t = (uchar**)pz_malloc((at+bt)*sizeof(uchar*)); // other strings
	tn = (uint*)pz_malloc((at+bt)*sizeof(uint*)); // other strings lengths

	i = 0;
	forn(k, at+bt+1){
		if(!strcmp("-wrt\0", argv[ps+k+1])){
			//DEBUG
	//		printf("skip -wrt\n");
			continue;
		}
		t[i] = (uchar*)argv[ps+k+1];
		tn[i] = strlen(argv[ps+k+1]) + 1;
		t[i][tn[i]-1] = 254;

		n = sn + tn[i]; // length of n + t_i
		st = (uchar*)pz_malloc(n*sizeof(uchar));
		p = (uint*)pz_malloc(n*sizeof(uint));
		r = (uint*)pz_malloc(n*sizeof(uint));
		h = (uint*)pz_malloc(n*sizeof(uint));

		memcpy(st, s, sn);
		memcpy(st+sn, t[i], tn[i]);
//		forn(j,n-sn-1) printf("%c", st[j+sn]); printf(" -- arg %d \n", i); //DEBUG

		bwt(NULL, p, r, st, n, NULL);
		forn(j,n) p[r[j]]=j;
		memcpy(h, r, n*sizeof(uint));
		lcp(n, st, h, p);

//		show_bwt_lcp(n, s, r, h);
		mcl(r, h, n, m, sn);
		if (i < at) csu(mc,m,sn);
		else opu(nc,m,sn);

		pz_free(h);
		pz_free(r);
		pz_free(p);
		pz_free(st);
		i++;
	}


	p = (uint*)pz_malloc(sn*sizeof(uint));
	r = (uint*)pz_malloc(sn*sizeof(uint));
	h = (uint*)pz_malloc(sn*sizeof(uint));

	bwt(NULL, p, r, s, sn, NULL);
	forn(j,sn) p[r[j]]=j;
	memcpy(h, r, sn*sizeof(uint));
	lcp(sn, s, h, p);

	// inhibit positions where tags do not start
	forn(i,sn) if (nc[i] >= mc[i]) nc[i] = -1;

	output_readable_data ord;
	ord.r = r;
	ord.s = s;
	ord.fp = stdout;

	output_callback *callback = output_readable_po;

	tag = 0;
	// finds the minimum tag
	forn(i,sn) if (nc[i] < nc[tag]) tag = i;

	if (all){
		printf("Shortest tags: (len = %d)\n", nc[tag]+1);
		forn(i,sn)
			if (nc[r[i]] == nc[tag] && (i == 0 || h[i] <= nc[tag]))
				callback(nc[r[i]]+1, i, 1, &ord);
	}

	else if (allm) {
		printf("Minimal tags:\n");
		forn(i,sn) {
//			printf("i = %u, nc[r[i]] = %u, nc[r[i]+1] = %u, h[i] = %u\n",
//									i, nc[r[i]], nc[r[i]+1], h[i]); //DEBUG
			if (nc[r[i]] != -1 && nc[r[i]] <= nc[r[i]+1] && (h[i] <= nc[r[i]]) )
				callback(nc[r[i]]+1, i, 1, &ord);
		}
	}

	else {

//	printf("pos: %d, len: %d\n", tag, nc[tag]+1); //DEBUG
	printf("Shortest tag: (len = %d)\n", nc[tag]+1);
	forn(i, nc[tag]+1) printf("%c",s[tag+i]);
	printf("\n"); //DEBUG

	}

	pz_free(p);
	pz_free(r);
	pz_free(h);
	pz_free(mc);
	pz_free(nc);
	pz_free(m);
	pz_free(t);
	pz_free(tn);

	return 0;
}
