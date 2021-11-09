#include <math.h>
#include <assert.h>

#include <strings.h>
#include <stdlib.h>

#include "bitarray.h"
#include "tipos.h"
#include "common.h"
#include "macros.h"

#include "comprsa.h"

#define __MAX_UINT 0xffffffff

#define bits_to_ba_bytes(X)	_pad_to_wordsize(_bits_to_bytes(X), 64)
#define bita_malloc_bits(X)	((word_type *)pz_malloc(bits_to_ba_bytes(X)))

#define RS_MIN_BITS 4

/*
 * bita_read(bitarray *ba, uint nbits, uint64 pos, uint value)
 * bita_write(bitarray *ba, uint nbits, uint64 pos, uint value)
 *
 * Reads (resp. writes) the <nbits> least significant bits of
 * <value> from (to) the position <pos> of the bitarray.
 *
 * Updates <pos> to the next position.
 */
/*** TODO: use safe shifts (replace "<<" and ">>") ***/
#define __min(X, Y)	((X) < (Y) ? (X) : (Y))
#define __max(X, Y)	((X) > (Y) ? (X) : (Y))

#define bita_read(ba, nbits, pos, res) { \
	uint __done; \
	(res) = 0; \
	if ((nbits)) { \
		(res) |= ((ba)[(pos) >> log_word_size] >> ((pos) & log_word_size_mask)); \
		(pos) += __done = __min((nbits), ba_word_size - ((pos) & log_word_size_mask)); \
		if (__done < (nbits)) { \
			(res) |= (((ba)[(pos) >> log_word_size] >> ((pos) & log_word_size_mask)) << __done); \
			(pos) += __min((nbits) - __done, ba_word_size - ((pos) & log_word_size_mask)); \
		} \
		(res) &= (1 << (nbits)) - 1; \
	} \
}

#define bita_write(ba, nbits, pos, value) { \
	uint __rd, __done; \
	if ((nbits)) { \
		__done = __min((nbits), ba_word_size - ((pos) & log_word_size_mask)); \
		(ba)[(pos) >> log_word_size] &= ~(((1 << __done) - 1) << ((pos) & log_word_size_mask)); \
		(ba)[(pos) >> log_word_size] |= ((value) << ((pos) & log_word_size_mask)); \
		(pos) += __done; \
		if (__done < (nbits)) { \
			__rd = __min((nbits) - __done, ba_word_size - ((pos) & log_word_size_mask)); \
			(ba)[(pos) >> log_word_size] &= ~(((1 << __rd) - 1) << ((pos) & log_word_size_mask)); \
			(ba)[(pos) >> log_word_size] |= (((value) >> __done) << ((pos) & log_word_size_mask)); \
			(pos) += __rd; \
		} \
	} \
}

/* Search in the space given by
 *   _block_value(_param, i, x)     (returns in <x> the <i>-th element)
 * with _l <= i < _u
 * - Update _l, _u to _l', _u' so that _l <= _l' + 1 == _u' <= _u
 *   and
 *   * _u' is the lowest index s.t. value(_u') NOT[OP] _find_val
 *         if there is any
 *   * _u' == _u  if not
 * - Execute _BLOCK when updating _l.
 */
#define midpoint(l, u)  (((l) >> 1) + ((u) >> 1) + (1 & (l) & (u)))
#define binsearch(OP, _l, _u, _m, _mid_val, _find_val, _BLOCK, _block_value, _param) { \
	while (_l + 1 < _u) { \
		_m = midpoint(_l, _u); \
		_block_value(_param, _m, _mid_val); \
		if (_mid_val OP _find_val) { \
			_l = _m; \
			_BLOCK \
		} else _u = _m; \
	} \
}

/***************** Extensible array ******************/

typedef uint *earray;
/* ea[0]   = alloc'd #elements
 * ea[1]   = real #elements
 * ea[2..] = elements */
#define ea_mem(EA) 			((EA)[0])
#define ea_len(EA) 			((EA)[1])
#define ea_free				pz_free
#define ea_at(EA, I) 			((EA)[(I) + 2])
#define ea_new_size(N, MAX_GROW) 	__min(2 * (N), (N) + (MAX_GROW))
#define ea_add(EA, VAL, MAX_GROW)	{ \
	if ((EA) == NULL) { \
		(EA) = pz_malloc(sizeof(uint) * 3); \
		ea_mem(EA) = 1; ea_len(EA) = 1; ea_at(EA, 0) = (VAL); \
	} else if (ea_len(EA) < ea_mem(EA)) { \
		ea_at(EA, ea_len(EA)) = (VAL); ++ea_len(EA); \
	} else { \
		uint __new_size = ea_new_size(ea_mem(EA), (MAX_GROW)); \
		uint *__new_ea, __i; \
		__new_ea = pz_malloc(sizeof(uint) * (2 + __new_size)); \
		ea_mem(__new_ea) = __new_size; \
		ea_len(__new_ea) = ea_len(EA) + 1; \
		forn (__i, ea_len(EA)) \
			ea_at(__new_ea, __i) = ea_at(EA, __i); \
		ea_at(__new_ea, ea_len(EA)) = (VAL); \
		pz_free((EA)); \
		(EA) = __new_ea; \
	} \
}

/***************** Trie ******************/

uint _trie_code[] = {
	/* 'A', 'C', 'G', 'N', 'T', 254, 255 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1,  0, -1,  1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1,  3, -1,
	-1, -1, -1, -1,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  5,  6,
};

/* Character used to pad the strings */
#define ALPHA_PAD 		((uchar)255)

typedef uchar trie_alpha;
#define TRIE_ALPHA_SIZE 	7
#define TRIE_LOG_ALPHA_SIZE 	3
#define TRIE_ORD(X) 		(_trie_code[(uint)(X)])

typedef earray trie_data;
#define trie_destroy_data(X)	if (X) { ea_free(X); }
#define trie_empty_data 	NULL

typedef struct _trie_node trie;
struct _trie_node {
	trie *children[TRIE_ALPHA_SIZE];
	trie_data data[];
};

trie *trie_new(uint ndata) {
	assert_(ndata <= 1);
	uint i;
	trie *t = pz_malloc(sizeof(trie) + sizeof(trie_data) * ndata);
	forn (i, TRIE_ALPHA_SIZE)
		t->children[i] = NULL;
	forn (i, ndata)
		t->data[i] = trie_empty_data;
	return t;
}

trie *trie_down(trie *t, trie_alpha c, uint ndata) {
	assert_(ndata <= 1);
	trie *ch;
	uint o = TRIE_ORD(c);
	assert(o != __MAX_UINT);
	if (!(ch = t->children[o])) {
		t->children[o] = ch = trie_new(ndata);
	}
	return ch;
}

/* Visits all the nodes of the trie, except for the root, in preorder.
 * For each node, executes BLOCK with TRIE_NODE bound to it. */
#define trie_it_depth()		((uint)__stack_index)
#define trie_it_key(I)		(__j_stack[(I)])
#define trie_iterate(TRIE, MAX_DEPTH, TRIE_NODE, BLOCK) { \
	trie *__t_stack[(MAX_DEPTH)]; \
	uint __j_stack[(MAX_DEPTH)]; \
	trie *TRIE_NODE; \
	int __stack_index = 0; \
	__t_stack[0] = TRIE; \
	__j_stack[0] = 0; \
	do { \
		/* Visit node */ \
		TRIE_NODE = __t_stack[__stack_index]->children[__j_stack[__stack_index]]; \
		if (TRIE_NODE) { \
			BLOCK \
			/* Push it */ \
			++__stack_index; \
			__t_stack[__stack_index] = TRIE_NODE; \
			__j_stack[__stack_index] = __MAX_UINT; \
		} \
		/* Find the next child */ \
		while (__stack_index >= 0 && ++__j_stack[__stack_index] >= TRIE_ALPHA_SIZE) \
			--__stack_index; \
	} while (__stack_index >= 0); \
}

void trie_destroy(trie *t) {
	uint i;
	trie *ch;
	bool has_children = FALSE;
	forn (i, TRIE_ALPHA_SIZE) {
		ch = t->children[i];
		if (ch) {
			trie_destroy(ch);
			has_children = TRUE;
		}
	}
	if (!has_children) {
		trie_destroy_data(t->data[0]);
	}
	pz_free(t);
}

/**** Compressed suffix array ****/

#define sa_pnkmo(sa)	((sa)->n_k - 1)

/* Prototypes */
uint sa_compressed_psi(sa_compressed *sa, uint i);
uint sa_inv_compressed_psi(uchar *s, sa_compressed *sa, uint ri, uint psi_i);
uint sa_psi(sa_compressed *sa, uint i);

#define SA_QUOT_CHUNK_SIZE	(ba_word_size - 1)
#define SA_REM_CHUNK_SIZE	(ba_word_size - 1)
#define bita_put(ba, pos, x)	if (x) { bita_set(ba, pos); } else { bita_unset(ba, pos); }
/*
 * s = source string
 * r = suffix array
 * n = length of s
 *
 * Note: DOESN'T free <s>
 *       DOES    free <r>
 * Returns a new sa_compressed, which doesn't alias <s> nor <r>
 */
#define SA_MAX_LEVELS		4
sa_compressed *sa_compress(uchar *s, uint *r, uint n) {
	if (s[n - 1] != ALPHA_PAD) {
		fprintf(stderr, "ERROR: sa_compress: source string does not end in 0x%.2x\n", ALPHA_PAD);
		return NULL;
	}

	uint k;

	assert(n > RS_MIN_BITS);
	uint levels = __min(SA_MAX_LEVELS, bneed(bneed(n) - 1) - 1);
	while ((n / (1 << (levels - 1))) <= RS_MIN_BITS) --levels;

	uint half_n = n;
	sa_compressed *sa_res, *sa = NULL;
	sa_compressed **sa_next = &sa_res;
	uint n_k = n;
	for (k = 1; k <= levels; ++k) {
		const uint log_pot_k = k - 1;
		const uint pot_k = 1 << log_pot_k;  /* pot_k == 2 ** (k - 1) */
		const bool pad = n_k & 1;
		half_n = n_k / 2 + pad;
		uint64 i;

#if __DEBUG_COMPSA

		uint *_p = pz_malloc(sizeof(uint) * n_k);
		forn (i, n_k) _p[r[i]] = i;
		uint *psi = pz_malloc(sizeof(uint) * n_k);
		/**** Direct definition of psi ****/
		forn (i, n_k) {
			if (pad && r[i] == n_k - 1) {
				psi[i] = n_k;
			} else {
				psi[i] = ((r[i] & 1) ? i : _p[r[i] + 1]);
			}
		}
		pz_free(_p);
#endif
		/**** Compressed definition of psi ****/
		trie *t = trie_new(0);
		/* Fill the trie. The concatenation of its leaves is the list
		 * "L_k", from which psi can be easily reconstructed. */
		uint j;
		trie *nt;
		uint idx;
		forn (i, n_k) if (r[i] & 1) {
			nt = t;
			uint ri = r[i];
			uint base = ((ri + 1) << log_pot_k) - 1;
			assert_(ri != 0);
			idx = base - pot_k;
			dforn (j, pot_k) {
				assert_(idx == base - j - 1);
				uchar c = (idx < n ? s[idx] : ALPHA_PAD);
				nt = trie_down(nt, c, (j == 0 ? 1 : 0));
				++idx;
			}
			ea_add(nt->data[0], i, n_k - i);
		}

#if __DEBUG_COMPSA
		uint *cpsi = pz_malloc(sizeof(uint) * (n_k / 2));
		uint cpsi_i = 0;
#endif

		/** Build a compressed representation of the list "L_k" **/
		const uint bneed_listid = TRIE_LOG_ALPHA_SIZE << log_pot_k;
		const uint bneed_elem = bneed(n_k - 1);
		const uint bneed_key = bneed_listid + bneed_elem;
		const uint bneed_quot = bneed(n_k / 2);
		const uint bneed_rem = bneed_key - bneed_quot;
		bita_declare(psi_key, bneed_key);

		/* To store quotients in unary */
		const uint64 size_quot = n_k + n_k / 2;
		bitarray *psi_quotients = bita_malloc_bits(size_quot);
		bzero(psi_quotients, bits_to_ba_bytes(size_quot));
		uint quotient, prev_quotient = 0;
		uint64 pos_quot = 0;
		/* To store the remainders */
		const uint64 size_rem = bneed_rem * (n_k / 2);
		bitarray *psi_remainders = bita_malloc_bits(size_rem);
		uint64 pos_rem = 0;

		trie_iterate(t, pot_k, nt, {
			if (trie_it_depth() == pot_k - 1) {
				/* Build the key: listid ++ elem

				 *           consisting of pot_k = (2 ** k) symbols
				 *           each one taking TRIE_LOG_ALPHA_SIZE bits
				 * - elem    is the current element, 0 <= elem < n_k
				 *           taking bneed(n_k - 1) bits
				 *
				 * For each "listid", its corresponding "elem" list is increasing.
				 * Therefore, "listid ++ elem" is increasing.
				 *
				 * The keys are stored LSB first.
				 */
				uint64 listid_pos = bneed_elem;
				assert_(trie_it_depth() == pot_k - 1);
				dforn (j, pot_k) {
					bita_write(psi_key, TRIE_LOG_ALPHA_SIZE, listid_pos, trie_it_key(j));
				}
				assert_(listid_pos == bneed_key);

				/* Concatenate the contents */
				forn (j, ea_len(nt->data[0])) {
					uint elem = ea_at(nt->data[0], j);

					debugging_compsa_( assert_(cpsi_i < n_k / 2); )
					debugging_compsa_( cpsi[cpsi_i++] = elem; )

					assert_(elem < n_k);
					uint64 elem_pos = 0;
					bita_write(psi_key, bneed_elem, elem_pos, elem);
					assert_(elem_pos = bneed_elem);

					/* Write quotient in unary */
					uint rd_pos = bneed_rem;
					bita_read(psi_key, bneed_quot, rd_pos, quotient);
					assert_(quotient >= prev_quotient);
					while (prev_quotient < quotient) {
						uint dif = __min(quotient - prev_quotient, SA_QUOT_CHUNK_SIZE);
						assert_(pos_quot + dif <= size_quot);
						bita_write(psi_quotients, dif, pos_quot, 0);
						prev_quotient += dif;
					}
					assert_(pos_quot < size_quot);
					bita_set(psi_quotients, pos_quot);
					++pos_quot;

					/* Write remainder in bneed_rem bits */
					rd_pos = bneed_rem;
					while (rd_pos > 0) {
						uint rem_i;
						uint rd = __min(rd_pos, SA_REM_CHUNK_SIZE);
						rd_pos -= rd;
						bita_read(psi_key, rd, rd_pos, rem_i);
						rd_pos -= rd;
						assert_(pos_rem + rd <= size_rem);
						bita_write(psi_remainders, rd, pos_rem, rem_i);
					}
				}
			}
		});
		trie_destroy(t);

		debugging_compsa_( assert_(cpsi_i == n_k / 2) );
		assert_(pos_quot <= size_quot);

#if 0
		if (pos_quot < size_quot) {
			/* Copy the quotients to an array that fits exactly */
			++pos_quot;
			bitarray *psi_quot_copy = bita_malloc_bits(pos_quot);
			memcpy(psi_quot_copy, psi_quotients, bits_to_ba_bytes(pos_quot));
			pz_free(psi_quotients);
			psi_quotients = psi_quot_copy;
		}
#endif

		/**** Definition of B ****/
		bitarray *b = bita_malloc_bits(n_k);
		bzero(b, bits_to_ba_bytes(n_k));
		uint64 bit_pos;
		forn (i, n_k) {
			bit_pos = i;
			bita_put(b, bit_pos, r[i] & 1);
		}

		/**** Halve the suffix array ****/
		/* The suffix array is compressed in the next iteration
		 * if there are more levels.
		 *
		 * If this is the last level, half_r is stored as
		 * part of the compressed suffix array.
		 */
		uint *half_r = pz_malloc(sizeof(uint) * half_n);
		j = 0;
		forn (i, n_k) {
			assert_(j <= half_n);
			if (r[i] & 1) half_r[j++] = r[i] / 2;
		}
		assert_(j == n_k / 2);
		if (pad) half_r[j++] = n_k / 2;
		assert_(j == half_n);
		pz_free(r);
		r = half_r;

		sa = pz_malloc(sizeof(sa_compressed));
		assert_(n_k > RS_MIN_BITS);

		sa->rsb = ranksel_create(n_k, b);

		assert_(size_quot > RS_MIN_BITS);

		sa->rs_quotients = ranksel_create(size_quot, psi_quotients);

		sa->remainders = psi_remainders;
		sa->bneed_rem = bneed_rem;
		sa->bneed_quot = bneed_quot;
		sa->bneed_key = bneed_key;
		sa->bneed_elem = bneed_elem;
		sa->n = n;	/* TODO: store only once? */
		sa->n_k = n_k;
		sa->pot_k = pot_k;
		sa->log_pot_k = log_pot_k;
		sa->pad = pad;
		sa->half_r = NULL;
		sa->half_p = NULL;
		sa->sa_next = NULL;
		*sa_next = sa;
		sa_next = &sa->sa_next;
#if __DEBUG_COMPSA
		sa->compressed_psi = cpsi;
		sa->psi = psi;
		sa->half_r = r;

		sa->half_p = pz_malloc(sizeof(uint) * half_n);
		forn (j, half_n)
			sa->half_p[sa->half_r[j]] = j;

		printf("CONTEXTO:\n");
		printf("L_k= ");
		forn (j, n_k / 2) {
			uint cpsi2 = sa_compressed_psi(sa, j);
			assert(cpsi2 == cpsi[j]);
			printf(" %.3i", cpsi[j]);
		}
		printf("\n");
		forn (j, n_k / 2) {
			printf("n_k= %i\n", n_k);
			uint ri = sa_lookup(sa, cpsi[j]) - 1;
			uint inv = sa_inv_compressed_psi(s, sa, ri, cpsi[j]);
			assert(inv == j);
		}
		pz_free(sa->half_p);
		sa->half_p = NULL;
		sa->half_r = NULL;
#endif
		n_k = half_n;
	}
	sa->half_r = r;
	uint i;
	sa->half_p = pz_malloc(sizeof(uint) * half_n);
	forn (i, half_n) sa->half_p[sa->half_r[i]] = i;

	return sa_res;
}

void sa_destroy(sa_compressed *sa) {
	if (sa->sa_next) sa_destroy(sa->sa_next);
	pz_free(sa->rsb->data);
	ranksel_destroy(sa->rsb);

	pz_free(sa->rs_quotients->data);
	ranksel_destroy(sa->rs_quotients);

	pz_free(sa->remainders);

#if __DEBUG_COMPSA
	pz_free(sa->compressed_psi);
	pz_free(sa->psi);
#endif
	if (sa->half_r) pz_free(sa->half_r);
	if (sa->half_p) pz_free(sa->half_p);
	pz_free(sa);
}

/**** Lookup ****/

uint sa_compressed_psi(sa_compressed *sa, uint i) {
	bita_declare(psi_key, sa->bneed_key);
	/* Quotient */
	uint wt_pos;
	if (sa->bneed_elem > sa->bneed_rem) {
		wt_pos = sa->bneed_rem;
		uint sel = ranksel_select1(sa->rs_quotients, i + 1);
		uint quotient = sel - i;
		assert_(i == ranksel_rank1(sa->rs_quotients, sel));
		assert_(quotient < (uint)(1 << sa->bneed_quot));
		bita_write(psi_key, sa->bneed_quot, wt_pos, quotient);
		assert_(wt_pos == sa->bneed_key);
	}
	/* Remainder */
	uint64 pos_rem = i * sa->bneed_rem;

	wt_pos = sa->bneed_rem;
	while (wt_pos > sa->bneed_elem + SA_REM_CHUNK_SIZE) {
		wt_pos -= SA_REM_CHUNK_SIZE;
		pos_rem += SA_REM_CHUNK_SIZE;
	}

	while (wt_pos > 0) {
		uint rem_i;
		uint wt = __min(wt_pos, SA_REM_CHUNK_SIZE);
		bita_read(sa->remainders, wt, pos_rem, rem_i);
		wt_pos -= wt;
		bita_write(psi_key, wt, wt_pos, rem_i);
		wt_pos -= wt;
	}
	assert_(wt_pos == 0);

	uint res;
	wt_pos = 0;
	bita_read(psi_key, sa->bneed_elem, wt_pos, res);
	assert_(wt_pos == sa->bneed_elem);
	return res;
}

uint sa_psi(sa_compressed *sa, uint i) {
	if (ranksel_get(sa->rsb, i)) {
		return i;
	} else if (sa->pad && i == sa_pnkmo(sa)) {
		return sa->n_k;
	} else {
		uint pos = ranksel_rank0(sa->rsb, i);
		uint cpsi = sa_compressed_psi(sa, pos);
		debugging_compsa_( assert_(sa->compressed_psi[pos] == cpsi) );
		return cpsi;
	}
}

#define sa_half_lookup(sa, j) \
	((sa)->half_r ?  (sa)->half_r[(j)] : sa_lookup((sa)->sa_next, (j)))

uint sa_lookup(sa_compressed *sa, uint i) {
	uint j = ranksel_rank1(sa->rsb, sa_psi(sa, i));
	uint hs = sa_half_lookup(sa, j);
	return 2 * hs + ranksel_get(sa->rsb, i);
}

/**** Inverse lookup ****/

uint sa_inv_compressed_psi(uchar *s, sa_compressed *sa, uint ri, uint psi_i) {
	/** Build key for i **/
	bita_declare(psi_key, sa->bneed_key);
	/* elem */
	uint64 pos = 0;
	assert_(psi_i < (1 << sa->bneed_elem));
	bita_write(psi_key, sa->bneed_elem, pos, psi_i);
	assert_(pos == sa->bneed_elem);
	/* listid = s[ (pot_k * ri - pot_k) .. (pot_k * ri - 1) ] */
	uint j;
	const uint rip = ri + 1;
	const uint base = ((rip + 1) << sa->log_pot_k) - 1;
	assert_(rip & 1);
	uint idx = base - 1;
	forn (j, sa->pot_k) {
		assert_(idx == base - j - 1);
		uint o = TRIE_ORD(idx < sa->n ? s[idx] : ALPHA_PAD);
		assert(o != __MAX_UINT);
		bita_write(psi_key, TRIE_LOG_ALPHA_SIZE, pos, o);
		--idx;
	}
	assert_(pos == sa->bneed_key);
	/** Index quotient and remainder **/
	uint quotient;
	pos = sa->bneed_rem;
	bita_read(psi_key, sa->bneed_quot, pos, quotient);

	const uint64 l_u = ranksel_select0_next(sa->rs_quotients, quotient);
	uint l = (quotient == 0) ? 0 : ((l_u >> 32) - quotient + 1);
	uint u = (l_u & 0xffffffff) - quotient;

	/* Find the remainder */
	uint rd_pos = sa->bneed_rem;
	while (l + 1 < u && rd_pos > 0) {
		/* For each piece */
		uint rd = __min(rd_pos, SA_REM_CHUNK_SIZE);
		uint rem_i;
		rd_pos -= rd;
		bita_read(psi_key, rd, rd_pos, rem_i);
		rd_pos -= rd;
		/* Update l, u to l', u' so that
		 *   sa->remainders[j][rd_pos..rd_pos+rd] == rem_i
		 *   for l <= l' <= j < u' <= u
		 * Invariant: sa->remainders[j][0..rd_pos] * is increasing in [l, u).
		 */
#define sa_read_remainder(sa, j, rem_j) { \
	uint64 __bpos = (j) * (sa)->bneed_rem + rd_pos; \
	bita_read((sa)->remainders, rd, __bpos, rem_j); \
}
		uint m, rem_m;
		uint tmp;
		tmp = u;
		binsearch(<, l, tmp, m, rem_m, rem_i, {}, sa_read_remainder, sa)
		binsearch(<=, l, u, m, rem_m, rem_i, {}, sa_read_remainder, sa)
#undef sa_read_remainder
	}
	return l;
}

#define sa_half_inv_lookup(s, sa, ri) \
	((sa)->half_p ?  (sa)->half_p[(ri)] : sa_inv_lookup((s), (sa)->sa_next, (ri)))

uint sa_inv_lookup(uchar *s, sa_compressed *sa, uint ri) {
	assert(ri < sa->n_k);
	uint hs = ri / 2;
	assert_(hs < sa->n_k / 2 + sa->pad);
	uint j = sa_half_inv_lookup(s, sa, hs);
	assert_(j < sa->n_k / 2 + sa->pad);
	assert_(sa_half_lookup(sa, j) == hs);
	assert_(sa->rsb->total_bits == sa->n_k);
	uint psi_i = ranksel_select1(sa->rsb, j + 1);
	assert_(j == ranksel_rank1(sa->rsb, psi_i));
	assert_(psi_i <= sa->n_k);
	if (ri & 1) {
		debugging_compsa_( assert_(sa->psi[psi_i] == psi_i); )
		return psi_i;
	} else if (sa->pad && psi_i == sa->n_k) {
		debugging_compsa_( assert_(sa->psi[sa_pnkmo(sa)] == psi_i); )
		return sa_pnkmo(sa);
	} else {
		uint pos = sa_inv_compressed_psi(s, sa, ri, psi_i);
		debugging_compsa_( assert_(sa->compressed_psi[pos] == psi_i); )
		assert_(sa_compressed_psi(sa, pos) == psi_i);
		uint i = ranksel_select0(sa->rsb, pos + 1);
		debugging_compsa_( assert_(sa->psi[i] == psi_i); )
		return i;
	}
}

#define _load(X) ret -= fread(&(X), sizeof(X), 1, fp) != 1
sa_compressed * sa_load(FILE * fp) {
	int ret = 0;
	sa_compressed * sa = pz_malloc(sizeof(sa_compressed));
	if (!sa) return NULL;

	/* Load several uints */
	_load(sa->bneed_rem);
	_load(sa->bneed_quot);
	_load(sa->bneed_key);
	_load(sa->bneed_elem);
	_load(sa->pad);
	_load(sa->n);
	_load(sa->n_k);
	_load(sa->pot_k);
	_load(sa->log_pot_k);

	sa->rsb = NULL;
	sa->rs_quotients = NULL;
	sa->sa_next = NULL;
	sa->half_r = NULL;
	sa->half_p = NULL;

	if (ret) {
		pz_free(sa);
		return NULL;
	}

	/* Load rank-sel: rsb */
	const uint64 b_rsb = bits_to_ba_bytes(sa->n_k);
	void* rsb_data = pz_malloc(b_rsb);
	ret -= rsb_data == NULL;
	ret -= fread(rsb_data, 1, b_rsb, fp) != b_rsb;
	sa->rsb = ranksel_load(rsb_data, fp);
	ret -= sa->rsb == NULL;

	/* Load rank-sel: quotients */
	const uint64 size_quot = sa->n_k + sa->n_k / 2;
	const uint64 b_quot = bits_to_ba_bytes(size_quot);
	void* rsq_data = pz_malloc(b_quot);
	ret -= rsq_data == NULL;
	ret -= fread(rsq_data, 1, b_quot, fp) != b_quot;
	sa->rs_quotients = ranksel_load(rsq_data, fp);
	ret -= sa->rs_quotients == NULL;

	/* Load the remainders bitarray */
	const uint64 size_rem = sa->bneed_rem * (sa->n_k / 2);
	const uint64 b_rem = bits_to_ba_bytes(size_rem);
	sa->remainders = pz_malloc(b_rem);
	ret -= sa->remainders == NULL;
	ret -= fread(sa->remainders, 1, b_rem, fp) != b_rem;

	/* Load the half_p/half_r  or  the next level */
	char l = '\0';
	ret -= fread(&l, 1, 1, fp) != 1;
	if (l == 'L') {
		/* Load another level */
		sa->sa_next = sa_load(fp);
		ret -= sa->sa_next == NULL;
	} else if (l == 'H') {
		/* Load sa->half_r + sa->half_p */
		const uint64 half_n = sa->n_k / 2 + sa->pad;
		sa->half_r = pz_malloc(sizeof(sa->half_r[0]) * half_n);
		ret -= fread(sa->half_r, sizeof(sa->half_r[0]), half_n, fp) != half_n;
		sa->half_p = pz_malloc(sizeof(sa->half_p[0]) * half_n);
		ret -= fread(sa->half_p, sizeof(sa->half_p[0]), half_n, fp) != half_n;
	} else {
		ret -= 1;
	}

	if (ret) {
		if (sa->rsb && sa->rsb->data) pz_free(sa->rsb->data);
		if (sa->rsb) ranksel_destroy(sa->rsb);

		if (sa->rs_quotients && sa->rs_quotients->data) pz_free(sa->rs_quotients->data);
		if (sa->rs_quotients) ranksel_destroy(sa->rs_quotients);

		if (sa->remainders) pz_free(sa->remainders);

		if (sa->sa_next) sa_destroy(sa->sa_next);
		if (sa->half_r) pz_free(sa->half_r);
		if (sa->half_p) pz_free(sa->half_p);
		pz_free(sa);
		return NULL;
	}
	return sa;
}
#undef _load

#define _store(X) ret -= fwrite(&(X), sizeof(X), 1, fp) != 1
int sa_store(sa_compressed * sa, FILE * fp) {
	int ret = 0;
	/* Store several uints */
	_store(sa->bneed_rem);
	_store(sa->bneed_quot);
	_store(sa->bneed_key);
	_store(sa->bneed_elem);
	_store(sa->pad);
	_store(sa->n);
	_store(sa->n_k);
	_store(sa->pot_k);
	_store(sa->log_pot_k);

	/* Store rank-sel: rsb */
	const uint64 b_rsb = bits_to_ba_bytes(sa->n_k);
	ret -= fwrite(ranksel_underlying_bitarray(sa->rsb), 1, b_rsb, fp) != b_rsb;
	ret -= ranksel_store(sa->rsb, fp) != 0;

	/* Store rank-sel: quotients */
	const uint64 size_quot = sa->n_k + sa->n_k / 2;
	const uint64 b_quot = bits_to_ba_bytes(size_quot);
	ret -= fwrite(ranksel_underlying_bitarray(sa->rs_quotients), 1, b_quot, fp) != b_quot;
	ret -= ranksel_store(sa->rs_quotients, fp) != 0;

	/* Store the remainders bitarray */
	const uint64 size_rem = sa->bneed_rem * (sa->n_k / 2);
	const uint64 b_rem = bits_to_ba_bytes(size_rem);
	ret -= fwrite(sa->remainders, 1, b_rem, fp) != b_rem;

	/* Stores the half_p/half_r  or  the next level */
	if (sa->sa_next) {
		/* Store another level */
		ret -= fwrite("L", 1, 1, fp) != 1;
		ret -= sa_store(sa->sa_next, fp) != 0;
	} else {
		/* Store sa->half_r + sa->half_p */
		const uint64 half_n = sa->n_k / 2 + sa->pad;
		ret -= fwrite("H", 1, 1, fp) != 1;
		ret -= fwrite(sa->half_r, sizeof(sa->half_r[0]), half_n, fp) != half_n;
		ret -= fwrite(sa->half_p, sizeof(sa->half_p[0]), half_n, fp) != half_n;
	}

	return ret;
}
#undef _store
