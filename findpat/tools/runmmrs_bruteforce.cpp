#include <cstdio>
#include <iostream>
#include <map>
#include <string>
using namespace std;
#define forn(i,n) for(int i=0;i<int(n);++i)
#define forsn(i,s,n) for(int i=s;i<int(n);++i)
#define dforsn(i,s,n) for(int i=int(n)-1;i>=s;--i)

int main(int argc, char** argv) {
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " <string>" << endl;
		return 1;
	}
	string s = argv[1];
	int n=s.size();
	map<string,int> reps;
	forn(i,n)forsn(l,1,n-i+1) ++reps[s.substr(i,l)];
	dforsn(l,1,n)forn(i,n-l+1) {
		string t = s.substr(i,l);
		if (reps[t] > 1) {
			cout << t << " (" << t.size() << ")" << endl;
			forn(it,t.size())forsn(lt,1,n-i) reps[t.substr(it,lt)]=0;
		}
	}
	return 0;
}
