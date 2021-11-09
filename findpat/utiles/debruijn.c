#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int uint;
typedef char Bool;

#define True 1
#define False 0

#define forsn(i,s,n)	for (i = (s); i < (n); i++)
#define forn(i,n)	forsn (i,0,n)

char *Alphabet = NULL;

Bool str_prev(char *s, uint len, uint m, uint start_change) {
	uint i;
	forsn (i, start_change + 1, len) s[i] = m;
	for (i = start_change; i > 0; i--) {
		if (s[i] != 0) {
			s[i]--;
			break;
		}
		s[i] = m;
	}
	return i != 0;
}

void str_print(char *s, uint len) {
	uint i;
	forn (i, len) printf("%c", Alphabet[s[i]]);
}

/* a string is Lyndon iff it is lexicographically greater than every rotation */
Bool lyndon(char *s, uint n) {
	uint i, j;
	forsn (i, 1, n) {
		for (j = 0; j < n; j++) {
			if (s[j] > s[(i + j) % n]) {
				break;
			} else if (s[j] < s[(i + j) % n]) {
				return False;
			}
		}
	}
	return True;
}

/* find the period of s, i.e. the min p such that
 *    s = y^k
 *    |y| = p
 */
uint min_period(char *s, uint n) {
	uint p, i;
	forsn (p, 1, n) {
		if (n % p != 0) continue;
		Bool p_periodic = True;
		forn (i, n) {
			if (s[i] != s[i % p]) {
				p_periodic = False;
				break;
			}
		}
		if (p_periodic) return p;
	}
	return n;
}

/*
 * generate all strings 'str' such that:
 *    str is Lyndon
 *    |str| == n
      str[0] == m
 * in lexicographically decreasing order
 *
 * for each of them produce the shortest y
 * such that
 *    str == y^k
 */
void all_necklaces(char *s, uint n, uint m) {
	uint i;
	forn (i, n) s[i] = m;
	for (;;) {
		if (lyndon(s, n)) {
			str_print(s, min_period(s, n));
		}
		if (!str_prev(s, n, m, n - 1)) break;
	}
}

void de_bruijn(char *s, uint n, uint m) {
	for (; m >= 1; m--) {
		all_necklaces(s, n, m);
	}
	s[0] = 0;
	str_print(s, 1);
	printf("\n");
}

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <alphabet> <n>\n", argv[0]);
		return 1;
	}
	Alphabet = argv[1];
	uint n = atoi(argv[2]);
	uint m = strlen(Alphabet) - 1;
	char *str = malloc(n);
	de_bruijn(str, n, m);
	return 0;
}

