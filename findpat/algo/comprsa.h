#ifndef __COMPRSA_H__
#define __COMPRSA_H__

#include "ranksel.h"

/*** Compressed suffix array ***/

#if __DEBUG_COMPSA
#define debugging_compsa_(X) 	X
#else
#define debugging_compsa_(X)
#endif

typedef struct _sa_compressed sa_compressed;
struct _sa_compressed {
	/* rsb holds a bitarray such that
	 * rsb[i] == 1    <==>  i-th position of the suffix array is odd */
	ranksel *rsb;
	/* Compressed psi */
	ranksel *rs_quotients;
	bitarray *remainders;
	uint bneed_rem, bneed_quot, bneed_key, bneed_elem;
	bool pad;
	uint n, n_k, pot_k, log_pot_k;
	/* Invariant:
	 *   (half_r == NULL) <==> (half_p == NULL)
	 *   (half_r == NULL) XOR (sa_next == NULL)
	 * <pad> flags if we are in the odd case.
	 */
	uint *half_r, *half_p;
	sa_compressed *sa_next;
#if __DEBUG_COMPSA
	uint *compressed_psi;
	uint *psi;
#endif
};

/* s = source string, _must_ end in 255
 * r = suffix array for <s>
 * n = length of <s> (and <r>)
 *
 * Creates a compressed representation for <r>,
 * destroying <r> in the process.
 *
 * Warning: it effectively makes a pz_free of <r>.
 */
sa_compressed *sa_compress(uchar *s, uint *r, uint n);

/* Calculates r[i]. */
uint sa_lookup(sa_compressed *sa, uint i);

/* Calculates p[i]. It requires the original string to be supplied. */
uint sa_inv_lookup(uchar *s, sa_compressed *sa, uint i);

/* Destroy and free the memory of <sa>. */
void sa_destroy(sa_compressed *sa);


/* Loads/stores the structure (and substructures) to/from a file. */
sa_compressed * sa_load(FILE * fp);
int sa_store(sa_compressed * sa, FILE * fp);

#endif /* __COMPRSA_H__ */
