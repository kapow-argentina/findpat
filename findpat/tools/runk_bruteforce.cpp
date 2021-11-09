#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdlib>
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

double k(const string& s, int base) {
	int vrf[s.size()];
	forn(i,s.size()) vrf[i] = rf(s,i);
	sort(vrf, vrf+s.size()); //This is only to decrease numerical problems
	double r = 0.0;
	forn(i,s.size()) r += log(vrf[i]+2)-log(vrf[i]+1);
	return r /= log(base);
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
