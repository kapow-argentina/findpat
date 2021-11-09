#ifndef __LONGPAT_H__
#define __LONGPAT_H__

#include "bittree.h"
#include "tipos.h"

bittree* longpat_bwttree(uchar *bwto, uint n);
void longpat_bwttree_free(bittree* bwtt, uint n);

typedef void(longpat_callback)(uint i, uint len, uint delta, void*);

void longpat(uchar* s, uint n, uint* r, uint* h, uint* p, bittree* bwtt,
	longpat_callback fun, void* data);

#endif
