#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>
#include <vector>
using namespace std;
#define forn(i,n) for(int i=0;i<int(n);++i)
#define forsn(i,s,n) for(int i=s;i<int(n);++i)
#define forall(i,c) for(typeof(c.begin()) i=c.begin();i!=c.end();++i)

int rnd(int n) { return rand()%n; }

int rf(const string& s, int i) {
	int mr = 0;
	forn(j,i) {
		if (s.substr(j-mr+1,mr)!=s.substr(i-mr+1,mr)) continue;
		while (j >= mr && s[i-mr] == s[j-mr]) ++mr;
	}
	return mr;
}

vector<double> B(const string& s, int base) {
	vector<double> vrf(s.size());
	forn(i,s.size()) vrf[i] = rf(s,i);
	vrf[0] = (log(vrf[0]+2)-log(vrf[0]+1)) / log(base);
	forsn(i,1,s.size()) vrf[i] = (log(vrf[i]+2)-log(vrf[i]+1))/log(base)+vrf[i-1];
	return vrf;
}


vector<double> Z(const string& s, int base) {
	vector<double> rz(s.size());
	double r = 0.0;
	forn(i,s.size()) {
		bool ok=false;
		forsn(l,1,s.size()-i+1) {
			rz[i+l-1]=r+1;
			if (s.find(s.substr(i,l)) >= (unsigned int)i) {
				i+=l-1;
				ok=true;
				break;
			}
		}
		r++;
		if (!ok) break;
	}
	return rz;
}

string rnds(int base, int n) {
	string r(n, ' ');
	forn(i,n) r[i]='0'+rnd(base);
	return r;
}

ostream& operator<<(ostream& o, const vector<double>& v) {
	forn(i,v.size()-1) o << v[i] << " ";
	return o << v[v.size()-1];
}

void showResults(const string& s, int base) {
	cout << s << endl;
	cout << B(s,base) << endl;
	cout << Z(s,base) << endl;
}

int main(int argc, char** argv) {
	srand(127);
	if (argc < 2 || argc > 4) {
		cerr << "Usage: " << argv[0] << " <string> [base]" << endl;
		cerr << "Usage: " << argv[0] << " -<num> [base] [cant]" << endl;
		return 1;
	}
	string s = argv[1];
	int base = argc > 2 ? atoi(argv[2]) : 2;
	if (base < 2) {
		cerr << "Usage: " << argv[0] << " <string> [base]" << endl;
		cerr << "Usage: " << argv[0] << " -<num> [base] [cant]" << endl;
		cerr << "base must be an integer greater than or equal to 2" << endl;
		return 1;
	}
	if (s[0] != '-') {
		if (argc > 3) {
			cerr << "Usage: " << argv[0] << " <string> [base]" << endl;
			cerr << "Usage: " << argv[0] << " -<num> [base] [cant]" << endl;
			return 1;
		}
		string t = s;
		sort(t.begin(), t.end());
		if (base < unique(t.begin(), t.end()) - t.begin()) {
			cerr << "Usage: " << argv[0] << " <string> [base]" << endl;
			cerr << "string must not contain more than base different characters" << endl;
			return 1;
		}
		showResults(s,base);
	} else {
		int l = atoi(s.substr(1).c_str());
		if (l < 1 || l > 1000000) {
			cerr << "Usage: " << argv[0] << " -<num> [base] [cant]" << endl;
			cerr << "num must be a base 10 integer between 1 and 1000000, inclusive" << endl;
			return 1;
		}
		int cant = argc > 3 ? atoi(argv[3]) : 0;
		if (cant < 0 || cant > 1000000) {
			cerr << "Usage: " << argv[0] << " -<num> [base] [cant]" << endl;
			cerr << "cant must be an integer between 1 and 1000000, inclusive" << endl;
			return 1;
		}
		if (cant == 0) {
			int c=1;
			forn(i,l) c*=base;
			forn(x,c/base) {
				string s(l, ' ');
				int xx=x;
				forn(i,l) s[i]='0'+(xx%base), xx/=base;
				showResults(s,base);
			}
		} else {
			set<string> ss;
			while(int(ss.size()) < cant) ss.insert(rnds(base, l));
			forall(it,ss) {
				const string& s = *it;
				showResults(s,base);
			}
		}
	}
	return 0;
}
