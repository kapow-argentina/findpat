#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cassert>
using namespace std;
#define forn(i,n) for(int i=0;i<int(n);++i)
#define forsn(i,s,n) for(int i=s;i<int(n);++i)

int bfbo(const string& s, int sz, int i) {
	int mr = 0, oc = 0;
	for(int j=i;j>=mr;--j) {
		if (s.substr(j-mr,mr)!=s.substr(i-mr+1,mr)) continue;
		oc++;
		while (j-mr-1 >= 0 && s[i-mr] == s[j-mr-1]) { ++mr; oc=1; }
	}
	return mr*oc;
}

int maxrect(const string& s, int sz, int i) {
	int mr = 0, oc = 0, r = 0;
	for(int j=i;j>=mr;--j) {
		if (s.substr(j-mr,mr)!=s.substr(i-mr+1,mr)) continue;
		oc++;
		r = max(r, oc*mr);
		while (j-mr-1 >= 0 && s[i-mr] == s[j-mr-1]) { ++mr; oc=1; }
		r = max(r, oc*mr);
	}
	return r;
}

bool check(const string& s, int base) {
	int l = 1, lb = base, maxn = base;
	while(maxn < int(s.size())) {
		l++;
		lb*=base;
		maxn=lb+l-1;
	}
	//assert(maxn == int(s.size()));
	bool esta[lb];
	forn(x,lb) esta[x]=false;
	forn(i,s.size()-l+1) {
		int id=0;
		forsn(j,i,i+l) id=id*base+s[j]-'0';
		if (esta[id]) return false;
		esta[id]=true;
	}
	return true;
}


int base = -1, len = -1;
string s;

void go(int i) {
	if (i>=len) {
		if (check(s,base)) cout << "! ";
		cout << s << endl;
	} else {
		int mx=i+1,cmx=0;
		int nxs[base];
		for(int x=0;x<base;x++) {
			s[i]='0'+x;
			nxs[x] = maxrect(s,i+1,i);
			if (mx==nxs[x]) cmx++; else if (mx>nxs[x]) { mx=nxs[x]; cmx=1; }
		}
		for(int x=0;x<base;x++) if (nxs[x] == mx) {
				s[i]='0'+x;
				if (cmx>1) cerr << "trying " << s << " (" << cmx << " with " <<  mx << ")" << endl;
				go(i+1);
		}
	}
}

int main(int argc, char** argv) {
	if (argc != 3) {
		cerr << "Usage: " << argv[0] << "<base> <len>" << endl;
		return 1;
	}
	base = atoi(argv[1]);
	if (base < 2) {
		cerr << "Usage: " << argv[0] << "<base> <len>" << endl;
		cerr << "base must be an integer greater than or equal to 2" << endl;
		return 1;
	}
	len = atoi(argv[2]);
	if (len < 2) {
		cerr << "Usage: " << argv[0] << "<base> <len>" << endl;
		cerr << "len must be an integer greater than or equal to 2" << endl;
		return 1;
	}
	s = string(len, ' ');
	s[0]='0';
	go(1);
	return 0;
}
