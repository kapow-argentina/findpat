
#include <math.h>
#include <assert.h>

#include <strings.h>
#include <stdlib.h>

#include "common.h"
#include "macros.h"

#include "ranksel.h"

#define ranksel_new_rs() pz_malloc(sizeof(ranksel))

#define L8 		0x0101010101010101
#define H8 		0x8080808080808080
#define LE8(X, Y) 	(((((Y) | H8) - ((X) & ~H8)) ^ (X) ^ (Y)) & H8)
#define POS8(X)		(((X) | (((X) | H8) - L8)) & H8)

#define uint64_count_1s_in_place(x) { \
	x = x - ((x & 0xaaaaaaaaaaaaaaaa) >> 1); \
	x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333); \
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f; \
	x = (x * L8) >> 56; \
}

#define ceil_div(x, y) 		(((x) + (y) - 1) / (y))
#define pdiv(x, y) 		((x) >> LOG_##y)
#define pmod(x, y) 		((x) & ((y) - 1))
#define pmul(x, y) 		((x) << LOG_##y)
#define WORD_SIZE 		32
#define LONG_WORD_SIZE 		64
#define LOG_LONG_WORD_SIZE 	6
#define bi2uint32(x)		ceil_div(x, WORD_SIZE)
#define bi2uint64(x)		ceil_div(x, LONG_WORD_SIZE)
#define RS_ACCMIN_BITS		9
#define RS_ACCMIN_MASK 		0x1ff	
#define RS_BLOCK_SIZE		8 
#define LOG_RS_BLOCK_SIZE 	3

#define longword_at(data, i) (((uint64)((data)[2 * (i) + 1]) << WORD_SIZE) | ((data)[2 * (i)]))
#define safe_longword_at(data, i, max) \
	( (2 * (i) + 1 < (max)) \
	? longword_at(data, i) \
	: ((2 * (i) < (max)) ? ((data)[2 * (i)]) : 0))

void _ranksel_create_rank(ranksel *rks, uint64 total_bits, bitarray *data) {
	const uint64 total_words = bi2uint32(total_bits);
	const uint64 total_long_words = bi2uint64(total_bits);
	const uint total_blocks = ceil_div(total_long_words, RS_BLOCK_SIZE);
	uint64 *rank = pz_malloc(sizeof(uint64) * 2 * (total_blocks + 1));

	uint block_i;
	uint64 accmaj = 0;
	uint di = 0;
	/* TODO: could unroll the border cases to avoid ifs */
	forn (block_i, total_blocks) {
		rank[2 * block_i] = accmaj;
		uint i, accmin = 0;
		uint64 accmin_lw = 0;
		forn (i, RS_BLOCK_SIZE) {
			uint64 val = safe_longword_at(data, di, total_words);
			uint64_count_1s_in_place(val);
			accmin += val;
			if (i != RS_BLOCK_SIZE - 1) {
				assert_(accmin < (1 << RS_ACCMIN_BITS));
				accmin_lw |= (uint64)accmin << (i * RS_ACCMIN_BITS);
			}
			++di;
		}
		rank[2 * block_i + 1] = accmin_lw;
		accmaj += accmin;
	}
	rank[2 * block_i] = accmaj;
	rank[2 * block_i + 1] = 0;
	rks->data = data;
	rks->rank = rank;

	rks->total_bits = total_bits;
	rks->total_ones = accmaj;
}

#define uint64_mask_lsb(cnt)	(((uint64)1 << (cnt)) - 1)

uint64 ranksel_rank1(ranksel *rks, uint64 pos) {
	const uint long_word = pdiv(pos, LONG_WORD_SIZE);
	const uint block_i = pdiv(long_word, RS_BLOCK_SIZE);
	uint64 res = rks->rank[2 * block_i];
	uint block_mod = pmod(long_word, RS_BLOCK_SIZE) - 1;
	block_mod += (block_mod >> 28) & 8; /* hack taken from the paper to avoid an if */
	res += ((rks->rank[2 * block_i + 1] >> (RS_ACCMIN_BITS * block_mod)) & RS_ACCMIN_MASK);
	uint64 val = longword_at(rks->data, long_word);
	val &= uint64_mask_lsb(pmod(pos, LONG_WORD_SIZE));
	uint64_count_1s_in_place(val);
	res += val;
	return res;
}

#define RS_INV_SET_BITS			4096
#define LOG_RS_INV_SET_BITS		12
#define RS_SUBINV_SET_BITS		256
#define LOG_RS_SUBINV_SET_BITS		8
#define SUBINVS_PER_INV			16
#define LOG_SUBINVS_PER_INV		6
#define RS_SUBINV_THRESHOLD 		65536
#define LOG_RS_SUBINV_THRESHOLD 	16

#define RS_long_bits_per_item		32
#define RS_long_items_per_uint64	2
#define RS_short_bits_per_item		16
#define RS_short_items_per_uint64	4

#define writebits_malloc(Items_per_uint64, Size) \
	pz_malloc(sizeof(uint64) * ceil_div((Size), Items_per_uint64))
#define writebits_free pz_free
#define writebits_start(Array) \
	uint64 *_array = (Array); uint64 _word = 0; uint _offset = 0;
#define writebits_push(Bits_per_item, Items_per_uint64, Value) { \
	assert_((Value) < ((uint64)1 << Bits_per_item)); \
	_word = ((Value) << ((Bits_per_item) * _offset)) | _word; \
	if (++_offset == Items_per_uint64) { \
		_offset = 0; \
		*_array++ = _word; \
		_word = 0; \
	} \
}
#define writebits_end() { if (_offset != 0) *_array = _word; }

/* XXX: works best for dense bitarrays. For sparse bitarrays,
 * it would be better to choose the size of the inventories and
 * subinventories depending on the density. */
#define __define_ranksel_create_select(NAME, SYMBOL, BIT_VALUE, TOTAL_SYMBOLS) \
void NAME(ranksel *rks) { \
	bitarray *data = rks->data; \
	uint64 total_bits = rks->total_bits; \
	uint64 total_ones = TOTAL_SYMBOLS(rks); \
	uint total_inventories = ceil_div(total_ones, RS_INV_SET_BITS) + 1; \
	bool inventory_pad = !!pmod(total_ones, RS_INV_SET_BITS); \
	uint *inventories = pz_malloc(sizeof(uint) * total_inventories); \
	uint64 **subinventories = pz_malloc(sizeof(uint64 *) * (total_inventories - 1)); \
	uint inv = 0, inv_ones = 0; \
	uint64 pos, inv_start_pos = 0; \
	forn (pos, total_bits) { \
		inv_ones += BIT_VALUE(!!bita_get(data, pos)); \
		uint inv_next_pos = pos + 1; \
		if (inv_ones == RS_INV_SET_BITS || (inventory_pad && inv_next_pos == total_bits)) { \
			inventories[inv] = inv_start_pos; \
			uint64 inv_size = inv_next_pos - inv_start_pos; \
			if (inv_size >= RS_SUBINV_THRESHOLD) { \
				/* Long inventory */ \
				subinventories[inv] = writebits_malloc(RS_long_items_per_uint64, inv_ones); \
				writebits_start(subinventories[inv]); \
				for (uint64 sub_pos = inv_start_pos; sub_pos < inv_next_pos; ++sub_pos) { \
					if (BIT_VALUE(bita_get(data, sub_pos))) { \
						writebits_push(RS_long_bits_per_item, RS_long_items_per_uint64, sub_pos); \
					} \
				} \
				writebits_end(); \
			} else { \
				/* Short inventory */ \
				subinventories[inv] = writebits_malloc(RS_short_items_per_uint64, ceil_div(inv_ones, RS_SUBINV_SET_BITS)); \
				bool subinventory_pad = !!pmod(inv_ones, RS_SUBINV_SET_BITS); \
				writebits_start(subinventories[inv]); \
				uint sub_ones = 0; \
				uint64 sub_start_pos = inv_start_pos; \
				for (uint64 sub_pos = inv_start_pos; sub_pos < inv_next_pos; ++sub_pos) { \
					sub_ones += BIT_VALUE(!!bita_get(data, sub_pos)); \
					uint sub_next_pos = sub_pos + 1; \
					if (sub_ones == RS_SUBINV_SET_BITS || (subinventory_pad && sub_next_pos == inv_next_pos)) { \
						writebits_push(RS_short_bits_per_item, RS_short_items_per_uint64, sub_start_pos - inv_start_pos); \
						sub_start_pos = sub_next_pos; \
						sub_ones = 0; \
					} \
				} \
				writebits_end(); \
			} \
			inv_start_pos = inv_next_pos; \
			++inv; \
			inv_ones = 0; \
		} \
	} \
	assert_(inv == total_inventories - 1); \
	inventories[inv] = inv_start_pos; \
	rks->sel[SYMBOL].total_inventories = total_inventories; \
	rks->sel[SYMBOL].inventories = inventories; \
	rks->sel[SYMBOL].subinventories = subinventories; \
}

#define long_subinv(A, I)	(((A)[(I) >> 1] >> (32 * ((I) & 0x1))) & 0xffffffff)
#define short_subinv(A, I)	(((A)[(I) >> 2] >> (16 * ((I) & 0x3))) & 0xffff)
#define __define_ranksel_select(NAME, SYMBOL, WORD_VALUE, TOTAL_SYMBOLS) \
uint64 NAME(ranksel *rks, uint64 cnt) { \
	assert_(cnt > 0); \
	if (cnt > TOTAL_SYMBOLS(rks)) return rks->total_bits; \
	--cnt; \
	const uint inv = pdiv(cnt, RS_INV_SET_BITS); \
	uint low = rks->sel[SYMBOL].inventories[inv]; \
	const uint up = rks->sel[SYMBOL].inventories[inv + 1]; \
	assert_(low < up); \
	if (up - low >= RS_SUBINV_THRESHOLD) { \
		return long_subinv(rks->sel[SYMBOL].subinventories[inv], pmod(cnt, RS_INV_SET_BITS)); \
	} else { \
		const bitarray *data = rks->data; \
		const uint64 subinv = pdiv(pmod(cnt, RS_INV_SET_BITS), RS_SUBINV_SET_BITS); \
		low += short_subinv(rks->sel[SYMBOL].subinventories[inv], subinv); \
		uint64 remain = pmod(cnt, RS_SUBINV_SET_BITS); \
		uint64 word_pos = pdiv(low, LONG_WORD_SIZE); \
		uint64 val = WORD_VALUE(longword_at(data, word_pos)); \
		val &= ~uint64_mask_lsb(pmod(low, LONG_WORD_SIZE)); \
		uint64 s; \
		for (;;) { \
			s = val - ((val & 0xaaaaaaaaaaaaaaaa) >> 1); \
			s = (s & 0x3333333333333333) + ((s >> 2) & 0x3333333333333333); \
			s = ((s + (s >> 4)) & 0x0f0f0f0f0f0f0f0f) * L8; \
			const uint64 done = s >> 56; \
			if (remain < done) break; \
			remain -= done; \
			++word_pos; \
			val = WORD_VALUE(longword_at(data, word_pos)); \
		} \
		uint b, l; \
		const uint64 r_L8 = remain * L8; \
		b = ((LE8(s, r_L8) >> 7) * L8 >> 53) & ~7; \
		l = remain - (((s << 8) >> b) & 0xff); \
		const uint64 xg = (val >> b & 0xff) * L8 & 0x8040201008040201; \
		s = (POS8(xg) >> 7) * L8; \
		const uint64 l_L8 = l * L8; \
		return pmul(word_pos, LONG_WORD_SIZE) + b + ((LE8(s, l_L8) >> 7) * L8 >> 56); \
	} \
}

#define IDENTITY(X)	(X)
#define NEGATE(X)	(!(X))
#define COMPLEMENT(X)	(~(X))
#define TOTAL_0s(rks)	((rks)->total_bits - (rks)->total_ones)
#define TOTAL_1s(rks)	((rks)->total_ones)
__define_ranksel_create_select(_ranksel_create_select0, 0, NEGATE, TOTAL_0s)
__define_ranksel_create_select(_ranksel_create_select1, 1, IDENTITY, TOTAL_1s)
__define_ranksel_select(ranksel_select0, 0, COMPLEMENT, TOTAL_0s)
__define_ranksel_select(ranksel_select1, 1, IDENTITY, TOTAL_1s)
#undef IDENTITY
#undef NEGATE
#undef COMPLEMENT
// #undef TOTAL_0s
// #undef TOTAL_1s

ranksel *ranksel_create(uint64 size_bits, bitarray *data) {
	ranksel *rks = ranksel_new_rs();
	_ranksel_create_rank(rks, size_bits, data);
	_ranksel_create_select0(rks);
	_ranksel_create_select1(rks);
	return rks;
}

void ranksel_destroy(ranksel *rks) {
	pz_free(rks->rank);
	uint i, j;
	forn (i, 2) {
		pz_free(rks->sel[i].inventories);
		forn (j, rks->sel[i].total_inventories - 1) {
			pz_free(rks->sel[i].subinventories[j]);
		}
		pz_free(rks->sel[i].subinventories);
	}
	pz_free(rks);
}

ranksel *ranksel_create_rank(uint64 size_bits, bitarray *data) {
	ranksel *rks = ranksel_new_rs();
	_ranksel_create_rank(rks, size_bits, data);
	rks->sel[0].total_inventories = 0;
	rks->sel[1].total_inventories = 0;
	return rks;
}

void ranksel_destroy_rank(ranksel *rks) {
	pz_free(rks->rank);
	pz_free(rks);
}

ranksel *ranksel_load(bitarray* data, FILE * fp) {
	ranksel *rks = ranksel_new_rs();
	int i, j, res = 1;
	res = fread(&rks->total_bits, sizeof(rks->total_bits), 1, fp) && res;
	res = fread(&rks->total_ones, sizeof(rks->total_ones), 1, fp) && res;
	if (!res) {
		pz_free(rks);
		return NULL;
	}
	rks->data = data;
	/* Load rank */
	const uint64 total_long_words = bi2uint64(rks->total_bits);
	const long total_blocks = ceil_div(total_long_words, RS_BLOCK_SIZE);
	const long ranksz = sizeof(uint64) * 2 * (total_blocks + 1);
	rks->rank = pz_malloc(ranksz);
	res = fread(rks->rank, 1, ranksz, fp) == ranksz && res;
	if (!res) {
		pz_free(rks->rank);
		pz_free(rks);
		return NULL;
	}
	/* Load rks->sel */
	forn(i, 2) {
		res = fread(&rks->sel[i].total_inventories, sizeof(rks->sel[i].total_inventories), 1, fp) == 1 && res;
		if (!rks->sel[i].total_inventories) continue; /* "sel" part of this structure is not used. */

		/* Load rks->sel[i].inventories */
		rks->sel[i].inventories = pz_malloc(sizeof(uint) * rks->sel[i].total_inventories);
		res = fread(rks->sel[i].inventories, sizeof(uint), rks->sel[i].total_inventories, fp) == rks->sel[i].total_inventories && res;
		/* Load rks->sel[i].subinventories[] */
		rks->sel[i].subinventories = (uint64**)pz_malloc(sizeof(uint64*) * (rks->sel[i].total_inventories-1));
		forn(j, rks->sel[i].total_inventories-1) {
			long wbitsz = 0;
			res = fread(&wbitsz, sizeof(long), 1, fp) == rks->sel[i].total_inventories && res;
			rks->sel[i].subinventories[j] = (uint64*)pz_malloc(wbitsz);
			res = fread(rks->sel[i].subinventories[j], 1, wbitsz, fp) == wbitsz && res;
		}
	}
	return rks;
}

int ranksel_store(ranksel *rks, FILE * fp) {
	int i;
	fwrite(&rks->total_bits, sizeof(rks->total_bits), 1, fp);
	fwrite(&rks->total_ones, sizeof(rks->total_ones), 1, fp);
	/* Store rank */
	const uint64 total_long_words = bi2uint64(rks->total_bits);
	const long total_blocks = ceil_div(total_long_words, RS_BLOCK_SIZE);
	const long ranksz = sizeof(uint64) * 2 * (total_blocks + 1);
	fwrite(rks->rank, ranksz, 1, fp);
	/* Store rks->sel */
	forn(i, 2) {
		fwrite(&rks->sel[i].total_inventories, sizeof(rks->sel[i].total_inventories), 1, fp);
		if (!rks->sel[i].total_inventories) continue; /* "sel" part of this structure is not used. */
		fwrite(rks->sel[i].inventories, sizeof(uint), rks->sel[i].total_inventories, fp);
		/* Store rks->sel[i].subinventories[] */
		uint64 total_ones = i?TOTAL_1s(rks):TOTAL_0s(rks);
		bool inventory_pad = !!pmod(total_ones, RS_INV_SET_BITS);
		uint inv = 0, inv_ones = 0;
		uint64 pos, inv_start_pos = 0;
		forn (pos, rks->total_bits) {
			inv_ones += i ^ (!bita_get(rks->data, pos));
			uint inv_next_pos = pos + 1;
			if (inv_ones == RS_INV_SET_BITS || (inventory_pad && inv_next_pos == rks->total_bits)) { 
				uint64 inv_size = inv_next_pos - inv_start_pos;
				if (inv_size >= RS_SUBINV_THRESHOLD) {
					/* Long inventory */ 
					long wbitsz = (sizeof(uint64) * ceil_div((inv_ones), RS_long_items_per_uint64));
					fwrite(&wbitsz, sizeof(wbitsz), 1, fp);
					fwrite(rks->sel[i].subinventories[inv], 1, wbitsz, fp);
				} else {
					/* Short inventory */
					long wbitsz = (sizeof(uint64) * ceil_div((ceil_div(inv_ones, RS_SUBINV_SET_BITS)), RS_short_items_per_uint64));
					fwrite(&wbitsz, sizeof(wbitsz), 1, fp);
					fwrite(rks->sel[i].subinventories[inv], 1, wbitsz, fp);
				}
				inv_start_pos = inv_next_pos;
				++inv;
				inv_ones = 0;
			}
		}
	}
	return 0;
}


#define WORD_VALUE(X)		(~(X))
#define SYMBOL 			0
#define TOTAL_SYMBOLS(rks)	((rks)->total_bits - (rks)->total_ones)
#define PAIR(X, Y) 		(((uint64)(X) << 32) | (Y))
uint64 ranksel_select0_next(ranksel *rks, uint64 cnt) {
#define _SIMPLE_SOLUTION (PAIR(ranksel_select0(rks, cnt + 1), ranksel_select0(rks, cnt + 2)))
	uint s_cnt = -1, s_next = -1;
	if (cnt == 0) return PAIR(0, ranksel_select0(rks, 1));
	assert_(cnt <= TOTAL_SYMBOLS(rks));
	assert_(cnt + 1 <= TOTAL_SYMBOLS(rks));
	--cnt;
	const uint inv = pdiv(cnt, RS_INV_SET_BITS);
	const uint next_inv = pdiv(cnt + 1, RS_INV_SET_BITS);
	if (inv != next_inv) return _SIMPLE_SOLUTION;

	uint low = rks->sel[SYMBOL].inventories[inv];
	const uint up = rks->sel[SYMBOL].inventories[inv + 1];
	assert_(low < up);
	if (up - low >= RS_SUBINV_THRESHOLD) {
		s_cnt = long_subinv(rks->sel[SYMBOL].subinventories[inv], pmod(cnt, RS_INV_SET_BITS));
		s_next = long_subinv(rks->sel[SYMBOL].subinventories[inv], pmod(cnt + 1, RS_INV_SET_BITS));
		return PAIR(s_cnt, s_next);
	} else {
		const bitarray *data = rks->data;
		const uint64 subinv = pdiv(pmod(cnt, RS_INV_SET_BITS), RS_SUBINV_SET_BITS);
		const uint64 next_subinv = pdiv(pmod(cnt + 1, RS_INV_SET_BITS), RS_SUBINV_SET_BITS);
		if (subinv != next_subinv) return _SIMPLE_SOLUTION;
		uint64 remain = pmod(cnt, RS_SUBINV_SET_BITS);
		low += short_subinv(rks->sel[SYMBOL].subinventories[inv], subinv);
		uint64 word_pos = pdiv(low, LONG_WORD_SIZE);
		uint64 val = WORD_VALUE(longword_at(data, word_pos));
		val &= ~uint64_mask_lsb(pmod(low, LONG_WORD_SIZE));
		uint64 s;
		uint64 done;

#define _LOCATE_BLOCK() { \
	for (;;) { \
		s = val - ((val & 0xaaaaaaaaaaaaaaaa) >> 1); \
		s = (s & 0x3333333333333333) + ((s >> 2) & 0x3333333333333333); \
		s = ((s + (s >> 4)) & 0x0f0f0f0f0f0f0f0f) * L8; \
		done = s >> 56; \
		if (remain < done) break; \
		remain -= done; \
		++word_pos; \
		val = WORD_VALUE(longword_at(data, word_pos)); \
	} \
}
#define _CALC_SELECT(RES) { \
	uint b, l; \
	const uint64 r_L8 = remain * L8; \
	b = ((LE8(s, r_L8) >> 7) * L8 >> 53) & ~7; \
	l = remain - (((s << 8) >> b) & 0xff); \
	const uint64 xg = (val >> b & 0xff) * L8 & 0x8040201008040201; \
	uint64 ss = (POS8(xg) >> 7) * L8; \
	const uint64 l_L8 = l * L8; \
	(RES) = pmul(word_pos, LONG_WORD_SIZE) + b + ((LE8(ss, l_L8) >> 7) * L8 >> 56); \
}

		_LOCATE_BLOCK();
		_CALC_SELECT(s_cnt);
		++remain;
		if (remain < done) {
			_CALC_SELECT(s_next);
			return PAIR(s_cnt, s_next);
		} else {
			remain -= done;
			++word_pos;
			val = WORD_VALUE(longword_at(data, word_pos));
		}
		_LOCATE_BLOCK();
		_CALC_SELECT(s_next);
		return PAIR(s_cnt, s_next);
	}
}
#undef WORD_VALUE
#undef SYMBOL
#undef TOTAL_SYMBOLS

