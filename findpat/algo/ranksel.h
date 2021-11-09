#ifndef __RANKSEL_H__
#define __RANKSEL_H__

#include "bitarray.h"
#include "tipos.h"

typedef struct {
	uint *inventories;
	uint64 **subinventories;
	uint64 total_inventories;
} rs_select;

typedef struct {
	bitarray *data;
	uint64 *rank;
	rs_select sel[2];
	uint64 total_bits, total_ones;
} ranksel;

ranksel *ranksel_create(uint64 size_bits, bitarray *data);
ranksel *ranksel_create_rank(uint64 size_bits, bitarray *data);
ranksel *ranksel_load(bitarray* data, FILE * fp);
int ranksel_store(ranksel *, FILE * fp);

#define ranksel_rank0(rks, pos) ((pos) - ranksel_rank1((rks), (pos)))
uint64 ranksel_rank1(ranksel *rks, uint64 pos);
uint64 ranksel_select0(ranksel *rks, uint64 cnt);
uint64 ranksel_select1(ranksel *rks, uint64 cnt);
void ranksel_destroy(ranksel *rks);
void ranksel_destroy_rank(ranksel *rks);

#define ranksel_underlying_bitarray(rks)	((rks)->data)

/*
 * Returns
 *   ranksel_select0(rks, cnt) ++ ranksel_select0(rks, cnt + 1)
 * optimized for the most common case.
 */
uint64 ranksel_select0_next(ranksel *rks, uint64 cnt);

#define ranksel_get(rks, i)	(bita_get((rks)->data, (i)))

#define __DEBUG_RANKSEL 	0
#define __DEBUG_COMPSA		0

#if __DEBUG_RANKSEL
#define assert_(X)	assert(X)
#else
#define assert_(X)
#endif

#endif /*__RANKSEL_H__*/
