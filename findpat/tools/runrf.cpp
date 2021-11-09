#include <iostream>
#include <string>
#include <algorithm>
using namespace std;
#define forn(i,n) for(int i=0;i<int(n);++i)
#define forsn(i,s,n) for(int i=s;i<int(n);++i)

int rf(const string& s, int i) {
	int mr = 0;
	forsn(j,i+1,s.size()-mr) {
		if (s.substr(j,mr)!=s.substr(i,mr)) continue;
		while (s[i+mr] == s[j+mr]) ++mr;
	}
	return mr;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " <string>" << endl;
		return 1;
	}
	string s = argv[1];
	int vrf[s.size()];
	int mrf = 0;
	forn(i,s.size()) mrf = max(mrf, vrf[i] = rf(s,i));
	int log10 = 1, p10 = 10;
	while(p10 <= mrf) { p10*=10; log10++; }
	forn(i,s.size()) cout << string(log10>1?log10:0, ' ') << s[i]; cout << endl;
	forn(i,s.size()) {
		cout.width(log10>1?log10+1:1);
		cout << vrf[i];
	} cout << endl;
	reverse(s.begin(), s.end());
	forn(i,s.size()) vrf[i] = rf(s,i);
	forn(i,s.size()) {
		cout.width(log10>1?log10+1:1);
		cout << vrf[s.size()-1-i];
	} cout << endl;
	return 0;
}
