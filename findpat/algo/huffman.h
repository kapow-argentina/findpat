#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#include "tipos.h"

#define HUFFMAN_MAX_CODELEN 64

typedef struct {
	uint64** vec;
	uint bused, zvec, nvec;
	uint64* buf;

} obstream;


/** hufman_compress_len()
 * Calculates the length in BITS of the stream src[0..size) compresed with
 * huffman. The length includes the codes used to describe the stream AND
 * the bits needed to describe the coding table.
 * alphaSize is the number of different codes. Each code must be in [0..alphaSize).
 * This parameter must be knowm by the decompressor, but size could be calculated.
 * If tableSize is not NULL, stores in *tableSize the amount of bits needed to
 * describe only the HuffmanTable.
 */
uint64 huffman_compress_len(ushort* src, uint size, uint alphaSize, uint* tableSize);

/** hufman_compress()
 * Compress the stream src[0..size) with huffman into the output-bit-stream obs.
 * The compressed stream contains the coding table described and the codes that
 * describes the buffer str.
 * alphaSize is the number of different codes. Each code must be in [0..alphaSize).
 * This parameter must be knowm by the decompressor, but size could be calculated.
 */
void huffman_compress(obstream* obs, ushort* src, uint size, uint alphaSize);

#endif
