#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bwt.h"
#include "lcp.h"
#include "cop.h"
#include "tipos.h"
#include "common.h"
#include "macros.h"
#include "mmrs.h"
#include "output_callbacks.h"
#include "mrs.h"
#include "tiempos.h"

#define TIME_RUN_INIT tiempo __t1,__t2;
#define TIME_RUN(var,op) { getTickTime(&__t1); { op; } getTickTime(&__t2); var = getTimeDiff(__t1, __t2); }
#define TIME_RUN_AC(var,op) { getTickTime(&__t1); { op; } getTickTime(&__t2); var += getTimeDiff(__t1, __t2); }


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
	TIME_RUN_INIT
	uint *p, *r, *h, *m, *mc, tn;
	uchar *s, *st, *t;
	uchar **filenames;
	uint sn,n,i,j,ml = 1, nm = 0, c = 0, v = 0, at = 0, time = 0;
	int ps = -1;
	filter_data fdata;
	double t_sarr = 0.0,t_lcp = 0.0,t_mcalc = 0.0,t_algo = 0.0;

	forsn(i, 1, argc) {
		if (0) {}
		else cmdline_opt_2(i, "-ml") { ml = atoi(argv[i]); }
		else cmdline_var(i, "nm", nm)
		else cmdline_var(i, "c", c)
		else cmdline_var(i, "v", v)
		else cmdline_var(i, "t", time)
		else {
			if (ps == -1) ps = i;
			if (ps+at != i) at = -argc-1;
			at++;
		}
	}
	
	if (at < 2 || (nm && c)) {
		fprintf(stderr, "Usage: %s <file> <file1> [<file2>] [<file3>]"
						" ... [options] \n"
						"  -nm will run mrs instead of mmrs\n"
						"  -ml <number> will use <number> as ml parameter\n"
						"  -c will find common patterns instead of exclusive (default)\n"
						"  -v gives more output in standard error (only to be used with pure text files)\n"
						"  -t calculates running times (no data output)\n"
						, argv[0]); 
		return 1;
	}
	
	filenames = (uchar**)pz_malloc(at*sizeof(uchar*));
	forn(i,at) filenames[i] = (uchar*)argv[ps+i];
	
	s = loadStrFileExtraSpace((const char*)filenames[0], &sn, 1);
	s[sn++] = 255;
	
	if (v) {
		fprintf(stderr, "Base string\n");
		forn(i,sn-1) fprintf(stderr, "%c", s[i]);
		fprintf(stderr, "\n");
	}

	mc = (uint*)pz_malloc(sn*sizeof(uint));
	if (c) {
		forn(i,sn) mc[i] = sn;
	} else {
		forn(i,sn) mc[i] = 0;
	}
	
	forsn(i, 1, at){
		t = loadStrFileExtraSpace((const char*)filenames[i], &tn, 1);
		t[tn++] = 254;
		if (v) {
			fprintf(stderr, "Rival %u\n", i);
			forn(j,tn-1) fprintf(stderr, "%c", t[j]);
			fprintf(stderr, "\n");
		}

		n = sn + tn;
		st = (uchar*)pz_malloc(n*sizeof(uchar));	
		memcpy(st, s, sn);
		memcpy(st+sn, t, tn);
		free(t);
		
		p = (uint*)pz_malloc(n*sizeof(uint));
		r = (uint*)pz_malloc(n*sizeof(uint));
		h = (uint*)pz_malloc(n*sizeof(uint));

		TIME_RUN_AC(t_sarr,bwt(NULL, p, r, st, n, NULL))
		forn(j,n) p[r[j]]=j;
		memcpy(h, r, n*sizeof(uint));
		TIME_RUN_AC(t_lcp,lcp(n, st, h, p))
		m = p; //place m on p to save memory
		TIME_RUN_AC(t_mcalc,mcl(r, h, n, m, sn))
		if (c) TIME_RUN_AC(t_mcalc,csu(mc,m,sn))
		else TIME_RUN_AC(t_mcalc,opu(mc,m,sn))

		pz_free(h);
		pz_free(r);
		pz_free(p);
		pz_free(st);
		
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

	output_callback *callback = time?output_nothing:output_readable_po;

	if (!c) {
		fdata.data = (void*) &ord;
		fdata.filter = mc;
		fdata.r = r;
		fdata.callback = callback;
		
		if (nm) TIME_RUN_AC(t_algo,mrs(s, sn, r, h, p, ml, own_filter_callback, &fdata))	
		else TIME_RUN_AC(t_algo,mmrs(s, sn, r, h, ml, own_filter_callback, &fdata))
	} else {	
		TIME_RUN_AC(t_algo,common_substrings(s, sn, r, mc, h, ml, callback, &ord));
	}
	
	if (time) {
		printf("         Suffix array calculations: %.2lf ms\n", t_sarr);
		printf("                  LCP calculations: %.2lf ms\n", t_lcp);
		printf("Maximum/minimum array calculations: %.2lf ms\n", t_mcalc);
		printf("                    Main algorithm: %.2lf ms\n", t_algo);
	}
	
	free(s);
	
	pz_free(p);
	pz_free(r);
	pz_free(h);
	pz_free(mc);
	pz_free(filenames);

	return 0;
}
