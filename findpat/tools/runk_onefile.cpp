#include <iostream>
#include <string>
#include <algorithm>
#include <cassert>
#include <cmath>
using namespace std;
#define forn(i,n) for(i=0;i<n;++i)
#define forsn(i,s,n) for(i=s;i<n;++i)
#define dforn(i,n) for(i=n-1;i>=0;--i)

#define MAXN 1000005

int n,t;
int *p, *r, *h;
string s;

void fix_index(int *b, int *e) {
	int pkm1, pk, np, i, d, m;
	pkm1 = p[*b+t];
	m = e-b; d = 0;
	np = b-r;
	forn(i, m) {
		if (((pk = p[*b+t]) != pkm1) && !(np <= pkm1 && pk < np+m)) {
			pkm1 = pk;
			d = i;
		}
		p[*(b++)] = np+d;
	}
}

bool comp(int i, int j) {
	return p[i+t] < p[j+t];
}

void suff_arr() {
	int i,j,bc[256];
	t = 1;
	forn(i,256) bc[i]=0;
	forn(i,n) ++bc[int(s[i])]; 
	forsn(i, 1, 256) { bc[i]+=bc[i-1]; }
	forn(i, n) r[--bc[int(s[i])]] = i;
	dforn(i, n) p[i]=bc[int(s[i])];
	for(t = 1; t <= n; t*=2) {
		for(i = 0, j = 1; i < n; i = j++) {
			while(j < n && p[r[j]] == p[r[i]]) ++j;
			if (j-i > 1) {
				sort(r+i, r+j, comp);
				fix_index(r+i, r+j);
			}
		}
	}
}

void lcp() {
	int hh = 0, i, j;
	forn(i,n) if (p[i] > 0) {
		j = r[p[i]-1];
		while(s[i+hh] == s[j+hh]) ++hh;
		h[p[i]-1] = hh;
		if (hh > 0) --hh;
	}
}

double k(const string& ss, int base) {
	int i;
	s = ss + "$";
	n = s.size();
	r = new int[n];
	p = new int[n];
	h = new int[n];	
	suff_arr();
	lcp();
	sort(h, h+n-1); //This is only to decrease numerical problems
	double res = 0.0;
	forn(i,n-1) res += log(h[i]+2)-log(h[i]+1);
	delete r;
	delete p;
	delete h;
	return res /= log(base);
}

int main(int argc, char** argv) {
	if (argc < 2 || argc > 3) {
		cerr << "Usage: " << argv[0] << "<string> [base]" << endl;
		return 1;
	}
	string s = argv[1];
	int base = argc > 2 ? atoi(argv[2]) : 2;
	if (base < 2) {
		cerr << "Usage: " << argv[0] << "<string> [base]" << endl;
		cerr << "base must be an integer greater than or equal to 2" << endl;
		return 1;
	}
	string t = s;
	sort(t.begin(), t.end());
	if (base < unique(t.begin(), t.end()) - t.begin()) {
		cerr << "Usage: " << argv[0] << "<string> [base]" << endl;
		cerr << "string must not contain more than base different characters" << endl;
		return 1;
	}
	cout << k(s, base) << endl;
	return 0;
}
