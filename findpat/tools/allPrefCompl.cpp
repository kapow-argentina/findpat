#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace std;
#define forn(i,n) for(int i=0;i<int(n);++i)

bool isNum(char* s) {
	while(*s) if (*s < '0' || *s > '9') return false; else ++s;
	return true;
}

int usage(char* p) {
	cerr << "Usage: " << p << " <alphabet>" << endl;
	cerr << "  alphabet must be an integer greater than or equal to 2" << endl;
	return -1;
}

double dlog(int a, int x) {
	return log(1.+1./x)/log(a);
}

double inv(int a, int x) {
	return 1./(x*log(a));
}

double f(int n) {
	return double(n)/log(n);
}

int main(int argc, char** argv) {
	if (argc != 2 || !isNum(argv[1])) return usage(argv[0]);
	int a = atoi(argv[1]);
	if (a < 2) return usage(argv[0]);
	int n=0,t;
	double comp=0.0;
	double mx=0.0,mx0=0.0,mx1=0.0,mx2=0.0;
	double sum=0.0;
	int k = 0;
	int bk = 1;
	while(cin >> t) {
		if (k<t) {
			k = max(t,k);
			bk *= a;
			sum += double(bk)/double(k*(k+1));
//cout << k << "," << sum << "," << double(bk)/(k+4) << endl;
		}
		++n;
		comp+=dlog(a,t+1);
		//cout << n << "," << comp << "," << log(n) << "," << comp/f(n) << endl; // << ","  << k << "," << comp*k/n << "," << comp*(k+1)/n << "," << comp*(k+2)/n << endl
		//comp = double(n)/double(k+1) + log(k+1)/log(a) + sum;
		//cout << n << "," << k << "," << comp << "," << f(n) << "," << comp/f(n) << ",,,," << inv(a,t+1) << "," << 2*(f(n)-f(n-1)) << endl;
		//cout << n << "," << k << "," << f(n) << "," << ((n-1)*2./log(n-1) + 1 / log(n-k-1)) << "," << ((n-1)*2./log(n-1) + 1 / log(n-k-1))/f(n) << endl;
		cout << n << "," << k << "," << f(n) << "," << (2.*f(n-1)+inv(a,k+1))/f(n) << endl;

		mx = max(mx, comp/f(n));
		mx0 = max(mx0, comp*k/n);
		mx1 = max(mx1, comp*(k+1)/n);
		mx2 = max(mx2, comp*(k+2)/n);
	}
	cout << mx << endl; //" " << mx0 << " " << mx1 << " " << mx2 << endl;
	return 0;
}
