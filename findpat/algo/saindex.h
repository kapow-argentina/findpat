#ifndef __SAINDEX_H__
#define __SAINDEX_H__

#include "tipos.h"
#include <stdio.h>

/*** Suffix Array based index structure ***/

typedef struct str_sa_index {
	uint n;
	uint flags;
	uint lookupsz;
	uchar *s;
	uint *r, *p;
	void *llcp, *rlcp, *h; /* Llcp and Rlcp arrays for MM search, and LCP array for binary search */
	uint *lk; /* Lookup table */
} sa_index;

/* init flags: */
#define SA_STORE_P   1
#define SA_NO_LRLCP  2
#define SA_DNA_ALPHA 4
#define SA_STORE_S   8
#define SA_STORE_LCP 16
#define SA_NO_SRC    32

/* s = source string (without terminator).
 * r = suffix array for <s>
 * n = length of <s> (and <r>)
 * lookup = prefix length for the lookup table.
 *
 * Creates a suffix array based index.
 * If SA_DNA_ALPHA is used, it makes a copy of s internaly. If not,
 * it copies a reference to s unless SA_STORE_S is used.
 *
 */
int saindex_init(sa_index* this, const uchar* s, uint n, uint lookup, uint flags);

/* Destroy and free the memory of <sa>. */
void saindex_destroy(sa_index *this);

/* Loads/stores the structure (and substructures) to/from a file. */
int saindex_load(sa_index* this, FILE * fp);
int saindex_store(sa_index* this, FILE * fp);

/* Debug show */
void saindex_show(const sa_index* this, FILE* fp);

/* Search using the Manber & Myers algorithm */
bool saindex_mmsearch(sa_index* this, uchar* pat, uint len, uint* vfrom, uint* vto);

#endif /* __COMPRSA_H__ */
