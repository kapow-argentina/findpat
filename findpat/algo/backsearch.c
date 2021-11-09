#include <string.h>
#include <assert.h>

#include "backsearch.h"

/** Alphabet **/

char Backsearch_alphabet[] = { 'A', 'C', 'G', 'N', 'T' }; /* Should be sorted */
int Backsearch_alphabet_inverse[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1,  0, -1,  1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1,  3, -1,  // @A .. O
	-1, -1, -1, -1,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // P .. Z*****
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
};

/** Backwards search */

void backsearch_init(char *s, uint *r, uint n, backsearch_data *backsearch) {
	uint i, j;

	backsearch->n = n;
	uint *count = backsearch->count;
	ranksel **rk = backsearch->rk;

	bitarray *symbol_in_bwt[Backsearch_alphabet_size];

	forn (j, Backsearch_alphabet_size) {
		symbol_in_bwt[j] = bita_malloc(n);
		bita_clear(symbol_in_bwt[j], n);
		count[j] = 0;
	}
	forn (i, n) {
		if (r[i] == 0) continue;
		int bwt_i = backsearch_char_code(s[r[i] - 1]);
		count[bwt_i]++;
		bita_set(symbol_in_bwt[bwt_i], i);
	}
	forn (j, Backsearch_alphabet_size) {
		rk[j] = ranksel_create_rank(n, symbol_in_bwt[j]);
	}
	uint acc = (uchar)s[n - 1] < (uchar)backsearch_code_char(0) ? 1 : 0;
	forn (j, Backsearch_alphabet_size + 1) {
		uint tmp = count[j];
		count[j] = acc;
		acc += tmp;
	}
}

bool backsearch_locate(backsearch_data *backsearch, char *t, uint m, uint *from, uint *to) {
	assert(m > 0);
	uint pb = 0;
	uint pe = backsearch->n;
	uint i;
	dforn (i, m) {
		int ti = backsearch_char_code(t[i]);
		assert(ti != -1);
		pb = backsearch->count[ti] + ranksel_rank1(backsearch->rk[ti], pb);
		pe = backsearch->count[ti] + ranksel_rank1(backsearch->rk[ti], pe);
		if (pe == pb || pe > backsearch->count[ti + 1]) {
			//printf("** not found: %s\n", &t[i]); break;
			*from = *to = 0;
			return FALSE;
		}
		//printf("** [%u, %u) in the suffix array: %s\n", pb, pe, &t[i]);
	}
	*from = pb;
	*to = pe;
	return TRUE;
}

void backsearch_destroy(backsearch_data *backsearch) {
	uint j;
	if (backsearch->n == 0) return;
	forn (j, Backsearch_alphabet_size) {
		pz_free(ranksel_underlying_bitarray(backsearch->rk[j]));
		ranksel_destroy_rank(backsearch->rk[j]);
	}
}

int backsearch_load(backsearch_data * bs, FILE * fp) {
	int result = 1, i;
	result = fread(&bs->n, sizeof(bs->n), 1, fp) == 1 && result;
	result = fread(bs->count, sizeof(bs->count[0]), Backsearch_alphabet_size+1, fp) == (Backsearch_alphabet_size + 1)  && result;
	if (!result) { bs->n = 0; return -1; }
	forn(i, Backsearch_alphabet_size) {
		/* Load bitarray */
		long bitasz = _bits_to_bytes(_pad_to_wordsize(bs->n, ba_word_size));
		void* data = pz_malloc(bitasz);
		result = fread(data, 1, bitasz, fp) == bitasz && result;
		if (!result) { bs->n = 0; pz_free(data); break; }
		/* Load rank-select */
		bs->rk[i] = ranksel_load(data, fp);
		result = result && bs->rk[i];
	}
	
	return result-1;
}

int backsearch_store(backsearch_data * bs, FILE * fp) {
	int result = 1, i;
	result = fwrite(&bs->n, sizeof(bs->n), 1, fp) == 1 && result;
	result = fwrite(bs->count, sizeof(bs->count[0]), Backsearch_alphabet_size+1, fp) == (Backsearch_alphabet_size + 1)  && result;
	forn(i, Backsearch_alphabet_size) {
		/* Store bitarray */
		long bitasz = _bits_to_bytes(_pad_to_wordsize(bs->n, ba_word_size));
		result = fwrite(ranksel_underlying_bitarray(bs->rk[i]), 1, bitasz, fp) == bitasz && result;
		/* Store rank-select */
		result = !ranksel_store(bs->rk[i], fp) && result;
	}
	return result-1;
}
