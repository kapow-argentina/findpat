#ifndef __BITTREE_H__
#define __BITTREE_H__

#include "bitarray.h"

typedef word_type* bittree;

bittree* bittree_malloc(uint n);
void bittree_free(bittree* tree, uint n);
void bittree_clear(bittree* tree, uint n);
void bittree_preset(bittree* tree, uint n, uint i);
void bittree_endset(bittree* tree, uint n);
void bittree_set(bittree* tree, uint n, uint i);
//void unset(bittree* tree, uint i);
/*IMPORTANT NOTE: if there is no item less than/greater than i, the results
 *are unpredictable
 */
uint bittree_max_less_than(bittree* tree, uint n, uint i);
uint bittree_min_greater_than(bittree* tree, uint n, uint i);

void bittree_show(bittree* tree, uint n);

#endif //__BITTREE_H__

