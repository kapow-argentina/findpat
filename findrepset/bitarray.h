#ifndef __BITARRAY_H__
#define __BITARRAY_H__

#include <string.h>

#include "tipos.h"

typedef uint word_type;
typedef word_type bitarray;
#define ba_word_size (8*sizeof(bitarray))
#define log_word_size 5
#define log_word_size_mask (ba_word_size-1)
#define bita_declare(ba, n) bitarray ba[((uint64)(n) + (uint64)ba_word_size - 1LL) / (uint64)ba_word_size]
#define bita_malloc(size) ((word_type*)pz_malloc((((size)+ba_word_size-1) & ~(uint)(ba_word_size-1))))

#define bita_clear(ba, n)  memset(ba, 0x00, ((n) + 7)/8)
#define bita_setall(ba, n) memset(ba, 0xFF, ((n) + 7)/8)

#define bita_get(ba, pos)  ((ba[(pos) >> log_word_size] >> ((pos) & log_word_size_mask)) & 1)
#define bita_unset(ba, pos) (ba[(pos) >> log_word_size] &= ~(1 << ((pos) & log_word_size_mask)))
#define bita_set(ba, pos)   (ba[(pos) >> log_word_size] |=  (1 << ((pos) & log_word_size_mask)))

#endif //__BITARRAY_H__
