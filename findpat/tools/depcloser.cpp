#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <string>
#include <cstdio>
using namespace std;

#define forall(it, X) for(typeof((X).begin()) it = (X).begin(); it != (X).end(); ++it)
#define esta(e, X) ((X).find(e) != (X).end())
#define all(X) (X).begin(), (X).end()

// Debug macros
#define DBG(X) cerr << #X << " = " << (X) << endl;
//template<class T> ostream& operator<<(ostream& o, vector<T> v) { o << "{ "; forall(it, v) o << (*it) << " "; o << "}"; return o;}
template<class T> ostream& operator<<(ostream& o, set<T> m) { o << "{ "; forall(it, m) o << (*it) << " "; o << "}"; return o;}
template<class T1, class T2> ostream& operator<<(ostream& o, map<T1, T2> m) { o << "{ "<< endl; forall(it, m) o << (it->first) << " --> " << (it->second) << endl; o << "}"; return o;}

typedef set<string> sstr;
typedef map<string, sstr > mapdep;

string objectize(const string& s) {
	int i = s.find_last_of('.');
	return s.substr(0, i) + ".o";
}
bool is_header(const string& s) { return s.size() && s[s.size()-1] == 'h'; }

mapdep deps;
sstr used, empty;
const sstr& depclose(const string& s) {
	if (!esta(s, deps)) return empty;
	if (esta(s, used)) return deps[s];
	used.insert(s);
	sstr res;
	sstr cur = deps[s];
	deps[s] = sstr();
	forall(it, cur) if (esta(*it, deps)) {
		res.insert(*it);
		const sstr& cl = depclose(*it);
		res.insert(all(cl));
	}
	return deps[s] = res;
}

void dumpdeps(const string& fn, const sstr dep) {
	int cp = fn.size() + 2;
	cout << fn << ":";
	forall(jt, dep) {
		if (cp+1+jt->size()+2 > 80) {
			cout << " \\" << endl << " "; cp = 1;
		}
		cout << " " << *jt; cp += jt->size()+1;
	}
	if (cp > 1) cout << endl;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		cerr << "Use: " << argv[0] << " <deps>" << endl;
		return 1;
	}
	freopen(argv[1], "rt", stdin);
	string fn;
	sstr exports;
	mapdep orgdeps;
	while (cin >> fn) {
		if (fn[fn.size()-1] != ':') cerr << "Unexpected: " << fn << endl;
		fn.resize(fn.size()-1);
		string ln;
		bool bar;
		deps[fn] = set<string>();
		do {
			if (!getline(cin, ln)) break;
			istringstream iss(ln);
			bar = false;
			string tkn;
			while (iss >> tkn) {
				if (tkn == "\\") { bar = true; break;}
				deps[fn].insert(objectize(tkn));
				orgdeps[fn].insert(tkn);
				if (tkn == "$(OBJS)") exports.insert(fn);
			}
		} while(bar);
	}
	/* Clausura transitiva */
	// Nota: Si A depende de B, pero B no está en la lista, B se elimina
	forall(it, deps) depclose(it->first);

	freopen(argv[1], "wt", stdout);

	/* Exporta dependencias originales */
	forall(it, orgdeps) if (!esta(it->first, exports)) {
		dumpdeps(it->first, it->second);
	}

	/* Exporta las dependencias de binarios */
	forall(it, exports) {
		dumpdeps(*it, deps[*it]);
	}
	
	return 0;
}
