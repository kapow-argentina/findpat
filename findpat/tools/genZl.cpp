#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace std;
#define forn(i,n) for(int i=0;i<int(n);++i)

bool isNum(char* s) {
	while(*s) if (*s < '0' || *s > '9') return false; else ++s;
	return true;
}

int usage(char* p) {
	cerr << "Usage: " << p << " <alphabet> <length>" << endl;
	cerr << "  alphabet must be an integer greater than or equal to 2" << endl;
	cerr << "  length must be an integer greater than or equal to 1" << endl;
	return -1;
}

int main(int argc, char** argv) {
	if (argc != 3 || !isNum(argv[1]) || !isNum(argv[2])) return usage(argv[0]);
	int a = atoi(argv[1]), l = atoi(argv[2]);
	if (a < 2 || l < 1) return usage(argv[0]);
	vector<int> zl(l);
	int k=0,sigkp1=a;
	forn(i,l) {
		if (i>=sigkp1+k) {
			++k;
			sigkp1*=a;
		}
		zl[i] = k;
	}
	forn(i,l) cout << (i?" ":"") << zl[i]; cout << endl;
	return 0;
}
