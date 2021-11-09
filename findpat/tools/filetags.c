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
#include "tiempos.h"

#define TIME_RUN_INIT tiempo __t1,__t2;
#define TIME_RUN(var,op) { getTickTime(&__t1); { op; } getTickTime(&__t2); var = getTimeDiff(__t1, __t2); }
#define TIME_RUN_AC(var,op) { getTickTime(&__t1); { op; } getTickTime(&__t2); var += getTimeDiff(__t1, __t2); }


int main(int argc, char** argv) {
	TIME_RUN_INIT
	uint *p, *r, *h, *m, *mc, *nc, tn;
	uchar *s, *st, *t;
	uchar **filenames;
	uint sn,n,i,j,k,wrt, all = 0, allm = 0, tag = 1, v = 0, time = 0;
	int ps = -1, at = -1, bt = 0;
	double t_sarr = 0.0,t_lcp = 0.0,t_mcalc = 0.0,t_algo = 0.0;

	wrt = 0;
	forsn(i, 1, argc) {
		if (0) {}
		else cmdline_opt_1(i, "-wrt") { wrt = 1; }
		else cmdline_opt_1(i, "-all") { all = 1; }
		else cmdline_opt_1(i, "-min") { allm = 1; }
		else cmdline_var(i, "v", v)
		else cmdline_var(i, "t", time)
		else {
			if (ps == -1) ps = i;
			if (wrt) bt++; // filter strings
			else at++;	// set strings
		}
	}

//	printf("base = %s, set = %d filter = %d\n", argv[1], at, bt); //DEBUG

	if (at < 0 || bt <= 0 || (all && allm)) {
		fprintf(stderr, "Usage: %s <file_1> <file_2> ... <file_n>\n"
						" -wrt <filter_file_1> <filter_file_2> ... <filter_file_m>\n"
						" [-all | -min]\n"
						"  -all shows all tags of minimum length\n"
						"  -min shows all minimal tags\n"
						"  -v gives more output in standard error (only to be used with pure text files)\n"
						"  -t calculates running times (no data output)\n"
						, argv[0]);
		return 1;
	}

	filenames = (uchar**)pz_malloc((at+bt+2)*sizeof(uchar*));
	filenames[0] = (uchar*)argv[ps];
	s = loadStrFileExtraSpace((const char*)filenames[0], &sn, 1);
	s[sn++] = 255;

	if (v) {
		fprintf(stderr, "Base string\n");
		forn(i,sn-1) fprintf(stderr, "%c", s[i]);
		fprintf(stderr, "\n");
	}

//	m = (uint*)pz_malloc(sn*sizeof(uint));

	mc = (uint*)pz_malloc(sn*sizeof(uint)); // array M (keep min)
	nc = (uint*)pz_malloc(sn*sizeof(uint)); // array N (keep max)
	forn(i,sn) mc[i] = sn;
	forn(i,sn) nc[i] = 0;


	i = 0;
	forn(k, at+bt+1){
		if(!strcmp("-wrt\0", argv[ps+k+1])){
			//DEBUG
	//		printf("skip -wrt\n");
			continue;
		}

		filenames[k+1] = (uchar*)argv[ps+k+1];
		t = loadStrFileExtraSpace((const char*)filenames[k+1], &tn, 1);
		t[tn++] = 254;
		if (v) {
			fprintf(stderr, "%s %u\n", (i < at)?"Rival":"Filtro", k+1);
			forn(j,tn-1) fprintf(stderr, "%c", t[j]);
			fprintf(stderr, "\n");
		}


		n = sn + tn; // length of n + t_i
		st = (uchar*)pz_malloc(n*sizeof(uchar));
		memcpy(st, s, sn);
		memcpy(st+sn, t, tn);
		free(t);
//		forn(j,n-sn-1) printf("%c", st[j+sn]); printf(" -- arg %d \n", i); //DEBUG

		p = (uint*)pz_malloc(n*sizeof(uint));
		r = (uint*)pz_malloc(n*sizeof(uint));
		h = (uint*)pz_malloc(n*sizeof(uint));

		TIME_RUN_AC(t_sarr,bwt(NULL, p, r, st, n, NULL))

		forn(j,n) p[r[j]]=j;
		memcpy(h, r, n*sizeof(uint));
		TIME_RUN_AC(t_lcp,lcp(n, st, h, p))

		m = p; //place m on p to save memory
		TIME_RUN_AC(t_mcalc,mcl(r, h, n, m, sn))

		if (i < at) TIME_RUN_AC(t_mcalc,csu(mc,m,sn))
		else TIME_RUN_AC(t_mcalc,opu(nc,m,sn))

		pz_free(h);
		pz_free(r);
		pz_free(p);
		pz_free(st);
		i++;
	}

	p = (uint*)pz_malloc(sn*sizeof(uint));
	r = (uint*)pz_malloc(sn*sizeof(uint));
	h = (uint*)pz_malloc(sn*sizeof(uint));

	TIME_RUN_AC(t_sarr,bwt(NULL, p, r, s, sn, NULL))
	forn(j,sn) p[r[j]]=j;
	memcpy(h, r, sn*sizeof(uint));
	TIME_RUN_AC(t_lcp,lcp(sn, s, h, p))

	output_readable_data ord;
	ord.r = r;
	ord.s = s;
	ord.fp = stdout;

	output_callback *callback = output_readable_po;

	// inhibit positions where tags do not start
	forn(i,sn) if (nc[i] >= mc[i]) nc[i] = -1;

	tag = 0;
	// finds the minimum tag
	TIME_RUN_AC(t_algo,forn(i,sn) if (nc[i] < nc[tag]) tag = i);

	if (all) {
		printf("Shortest tags: (len = %d)\n", nc[tag]+1);
		TIME_RUN_AC(t_algo,
			forn(i,sn) {
				if (nc[r[i]] == nc[tag] && (i == 0 || h[i] <= nc[tag]))
					callback(nc[r[i]]+1, i, 1, &ord);
				}
		);
	}

	else if (allm) {
		printf("Minimal tags:\n");
		TIME_RUN_AC(t_algo,
			forn(i,sn) {
				if (nc[r[i]] != -1 && nc[r[i]] <= nc[r[i]+1] && (h[i] <= nc[r[i]]))
					callback(nc[r[i]]+1, i, 1, &ord);
			}
		);
	}

	else {
		//	printf("pos: %d, len: %d\n", tag, nc[tag]+1); //DEBUG
		printf("Shortest tag: (len = %d)\n", nc[tag]+1);
		forn(i, nc[tag]+1)  { printf("%c",s[tag+i]); }
		printf("\n"); //DEBUG
	}

	if (time) {
		printf("         Suffix array calculations: %.2lf ms\n", t_sarr);
		printf("                  LCP calculations: %.2lf ms\n", t_lcp);
		printf("Maximum/minimum array calculations: %.2lf ms\n", t_mcalc);
		printf("                    Main algorithm: %.2lf ms\n", t_algo);
	}

	pz_free(p);
	pz_free(r);
	pz_free(h);
	pz_free(mc);
	pz_free(nc);
	pz_free(filenames);

	return 0;
}
