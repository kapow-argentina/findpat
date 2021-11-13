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
	uint *p, *r, *h, *m, *mc, *tn;
	uchar *s, *st;
	uchar **t;
	uint sn,n,i,j,ml = 1, nm = 0, c = 0;
	int ps = -1, at = -1;

	forsn(i, 1, argc) {
		if (0) {}
		else cmdline_opt_2(i, "-ml") { ml = atoi(argv[i]); }
		else cmdline_var(i, "nm", nm)
		else cmdline_var(i, "c", c)
		else {
			if (ps == -1) ps = i;
			at++;
		}
	}
	
	if (at < 1 || (nm && c)) {
		fprintf(stderr, "Usage: %s <string> <string1> [<string2>] [<string3>]"
						" ... [-ml ml] [-nm] \n"
						"  -nm will run mrs instead of mmrs\n"
						"  -ml <number> will use <number> as ml parameter\n"
						"  -c will find common patterns instead of own (default)\n"
						, argv[0]); 
		return 1;
	}
	
	s = (uchar*)argv[ps];
	sn = strlen(argv[ps]) + 1;
	s[sn-1] = 255;

	m = (uint*)pz_malloc(sn*sizeof(uint));
	mc = (uint*)pz_malloc(sn*sizeof(uint));
	if (c) {
		forn(i,sn) mc[i] = sn;
	} else {
		forn(i,sn) mc[i] = 0;
	}
	

	t = (uchar**)pz_malloc(at*sizeof(uchar*));
	tn = (uint*)pz_malloc(at*sizeof(uint*));
	
	forn(i, at){
		t[i] = (uchar*)argv[ps+i+1];
		tn[i] = strlen(argv[ps+i+1]) + 1;
		t[i][tn[i]-1] = 254;	

		n = sn + tn[i];
		st = (uchar*)pz_malloc(n*sizeof(uchar));	
		p = (uint*)pz_malloc(n*sizeof(uint));
		r = (uint*)pz_malloc(n*sizeof(uint));
		h = (uint*)pz_malloc(n*sizeof(uint));

		memcpy(st, s, sn);
		memcpy(st+sn, t[i], tn[i]);

		bwt(NULL, p, r, st, n, NULL);
		forn(j,n) p[r[j]]=j;
		memcpy(h, r, n*sizeof(uint));
		lcp(n, st, h, p);
	
//		show_bwt_lcp(n, s, r, h);
		mcl(r, h, n, m, sn);
		if (c) csu(mc,m,sn); else opu(mc,m,sn);

		/* TODO: an output function saying something more */
/*		forn(j, sn) printf("%d ", m[j]);
		printf("\n");*/
	
		pz_free(h);
		pz_free(r);
		pz_free(p);
		pz_free(st);
	}

/*
	printf("\n");
	forn(j, sn) printf("%u ", mo[j]);
	printf("\n");

	printf("\n");
	forn(j, sn) printf("%u ", mc[j]);
	printf("\n");*/
	
	p = (uint*)pz_malloc(sn*sizeof(uint));
	r = (uint*)pz_malloc(sn*sizeof(uint));
	h = (uint*)pz_malloc(sn*sizeof(uint));
	
	bwt(NULL, p, r, s, sn, NULL);
	forn(j,sn) p[r[j]]=j;
	memcpy(h, r, sn*sizeof(uint));
	lcp(sn, s, h, p);

	output_readable_data ord;
	ord.r = r;
	ord.s = s;
	ord.fp = stdout;

	output_callback *callback = output_readable_po;

	if (!c) {
		filter_data fdata;
		fdata.data = (void*) &ord;
		fdata.filter = mc;
		fdata.r = r;
		fdata.callback = callback;
		
		if (nm) mrs(s, sn, r, h, p, ml, own_filter_callback, &fdata);	
		else mmrs(s, sn, r, h, ml, own_filter_callback, &fdata);
	} else {	
		common_substrings(s, sn, r, mc, h, ml, callback, &ord);
	}
	
	pz_free(p);
	pz_free(r);
	pz_free(h);
	pz_free(mc);
	pz_free(m);
	pz_free(t);
	pz_free(tn);

	return 0;
}
