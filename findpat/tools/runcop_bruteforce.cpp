#include <string>
#include <iostream>
#include <set>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>




#define bool bool //prevent redefinition

#include "bwt.h"
#include "lcp.h"
#include "cop.h"
#include "tipos.h"
#include "common.h"
#include "macros.c"
#include "mmrs.h"
#include "output_callbacks.h"
#include "mrs.h"


//WARNING: chanchada inmunda

#include "bwt.c"
#include "lcp.c"
#include "cop.c"
#include "bittree.c"
#include "mmrs.c"
#include "output_callbacks.c"
#include "mrs.c"

void inters(set<string>& s1, const set<string>& s2) {
	for(set<string>::iterator it = s1.begin(); it != s1.end(); ) {
			if (s2.find(*it) == s2.end()) {
				if (it == s1.begin()) {
					s1.erase(it);
					it = s1.begin();
				} else {
					set<string>::iterator auxit = it--;
					s1.erase(auxit);
					++it;
				} 
			} else ++it;
	}
}

void show_bwt_lcp(uint n, uchar* src, uint* r, uint* h) {
	uint i, j;
	printf("\n");
	printf(" i  r[] lcp \n");
	forn(i, n) {
		printf("%3d %3d %3d ", i, r[i], h[i]);
		forn(j, n) printf("%c", src[(r[i]+j)%n]);
		printf("\n");
	}
}

struct bf_filter_data {
	string t;
	uchar* s;
	uint* r;
	output_callback* callback;
	void* data;
};

void own_bf_filter_callback(uint l, uint i, uint n, void* fdata){
	bf_filter_data* bfdata = (bf_filter_data*)fdata;
	string p(l, ' ');
	uint j;
	forn(j,l) p[j] = bfdata->s[bfdata->r[i]+j];
	if (bfdata->t.find(p) == -1) {
		bfdata->callback(l, i, n, bfdata->data);
	}
}

int main(int argc, char** argv) {
	uint *p, *r, *h;
	uchar *s;
	uint sn,j,ml = 1, nm = 0, c = 0;
	int ps = -1, at = -1, i, ip;

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
	
	if (at < 1) {
		fprintf(stderr, "Usage: %s <string> <string1> [<string2>] [<string3>]"
						" ... [-ml ml] [-nm] \n"
						"  -nm will run mrs instead of mmrs\n"
						"  -ml <number> will use <number> as ml parameter\n"
						"  -c will find common patterns instead of own (default)\n"
						, argv[0]); 
		return 1;
	}
	
	if (!c) {
		s = (uchar*)argv[ps];
		sn = strlen(argv[ps]) + 1;
		s[sn-1] = 255;

		string tt;

		forn(i, at)	{
			tt += argv[ps+i+1]; 
			tt += char(254);
		}

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

		bf_filter_data bfdata;
		bfdata.data = (void*) &ord;
		bfdata.t = tt;
		bfdata.r = r;
		bfdata.s = s;
		bfdata.callback = output_readable_po; 
		
		if (nm) mrs(s, sn, r, h, p, ml, own_bf_filter_callback, &bfdata);	
		else mmrs(s, sn, r, h, ml, own_bf_filter_callback, &bfdata);	

		pz_free(p);
		pz_free(r);
		pz_free(h);
	} else {
		
		string t;
		set<string> ss;
		bool f = true;
		forn(ip, at+1) {
			
			t = argv[ps+ip];
			set<string> st;
			forn(i,t.size())forsn(j,ml,t.size()-i+1) st.insert(t.substr(i,j));
			
			if (f) {
				f=false;
				ss = st;
			} else {
				inters(ss, st);
			}
		}
		
		set<string> sr = ss;
		for(set<string>::iterator it = ss.begin(); it != ss.end(); ++it) {
			sr.erase(it->substr(0,it->size()-1));
			sr.erase(it->substr(1));
		}
		
		for(set<string>::iterator it = sr.begin(); it != sr.end(); ++it) {
			cout << *it << endl;
		}
	}

	return 0;
}
