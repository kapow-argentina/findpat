#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cassert>
#include <vector>
using namespace std;
#define forn(i,n) for(int i=0;i<int(n);++i)
#define forsn(i,s,n) for(int i=s;i<int(n);++i)

int bf(const string& s, int sz, int i) {
	int mr = 0;
	for(int j=i;j>mr;--j) {
		if (s.substr(j-mr,mr)!=s.substr(i-mr+1,mr)) continue;
		while (s[i-mr] == s[j-mr-1]) ++mr;
	}
	return mr;
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
vector<int> cnt;
vector<int> last;

void go(int i) {
	if (i>=len) {
		if (check(s,base)) cout << "! "; else cout << "  ";
		cout << s << "  ";
		forn(i,len) if (last[i]==-1) break; else cout << " " << last[i];
		cout << endl;
	} else {
		int mx=i+1,cmx=0;
		int nxs[base];
		for(int x=0;x<base;x++) {
			s[i]='0'+x;
			nxs[x] = bf(s,i+1,i);
			if (mx==nxs[x]) cmx++; else if (mx>nxs[x]) { mx=nxs[x]; cmx=1; }
		}
		if ((--cnt[mx]) == 0) last[mx] = i;
		for(int x=0;x<base;x++) if (nxs[x] == mx) {
				s[i]='0'+x;
				if (cmx>1) cerr << "trying " << s << " (" << cmx << " with " <<  mx << ")" << endl;
				go(i+1);
		}
		++cnt[mx];
	}
}

int main(int argc, char** argv) {
	if (argc != 3) {
		cerr << "Usage: " << argv[0] << "<base> <len>" << endl;
		return 1;
	}
	base = atoi(argv[1]);
	if (base < 2) {
		cerr << "Usage: " << argv[0] << " <base> <len>" << endl;
		cerr << "base must be an integer greater than or equal to 2" << endl;
		return 1;
	}
	len = atoi(argv[2]);
	if (len < 2) {
		cerr << "Usage: " << argv[0] << " <base> <len>" << endl;
		cerr << "len must be an integer greater than or equal to 2" << endl;
		return 1;
	}
	s = string(len, ' ');
	s[0]='0';
	cnt.resize(len);
	last.resize(len);
	int bp = base;
	forn(i,len) {
		cnt[i] = bp - bp/base + 1;
		bp*=base;
		last[i] = -1;
	}
	--cnt[0];
	go(1);
	return 0;
}
