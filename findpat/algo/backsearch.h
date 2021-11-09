#ifndef __BACKSEARCH_H__
#define __BACKSEARCH_H__

#include "tipos.h"
#include "macros.h"
#include "ranksel.h"

/** Alphabet **/

#define Backsearch_alphabet_size	5

extern char Backsearch_alphabet[Backsearch_alphabet_size];
extern int Backsearch_alphabet_inverse[];

#define backsearch_code_char(Code)	(Backsearch_alphabet[(Code)])
#define backsearch_char_code(Char)	(Backsearch_alphabet_inverse[(unsigned char)(Char)])

/** Backwards search **/

typedef struct _backsearch_data backsearch_data;
struct _backsearch_data {
	uint n;
	uint count[Backsearch_alphabet_size + 1];
	ranksel *rk[Backsearch_alphabet_size];
};

/* Initialize the structure for the backwards search.
	Input:
		s = source string, including a terminator
		r = suffix array of s
		n = length of s, including the terminator

	Output:
		backsearch->count[symb] ==
			number of positions in s that contain a symbol < symb
		ranksel_rank1(backsearch->rk[symb], i) ==
			number of occurrences of symb in bwt[0..i)
			where bwt is the BWT of s
*/
void backsearch_init(char *s, uint *r, uint n, backsearch_data *backsearch);

/*
   Search the occurrences of the string t
   in the source string with which "backsearch"
   was initialized.

   m should be the length of t.

   returns the interval [from, to) of the suffix array
   returns TRUE iff found
*/
bool backsearch_locate(backsearch_data *backsearch, char *t, uint m, uint *from, uint *to);

/* Frees the structure */
void backsearch_destroy(backsearch_data *backsearch);

/* Loads/stores the structure (and substructures) to/from a file. */
int backsearch_load(backsearch_data * bs, FILE * fp);
int backsearch_store(backsearch_data * bs, FILE * fp);

#endif /* __BACKSEARCH_H__ */

