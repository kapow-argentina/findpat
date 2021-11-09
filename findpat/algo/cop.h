#ifndef __COP_H__
#define __COP_H__

#include "tipos.h"
#include "output_callbacks.h"
#include <stdio.h>


typedef struct filter_data {
void* data;
uint* filter;
uint* r;
output_callback* callback;
} filter_data;

/**
 * Own Patterns Filter Callback
 *
 * An output_callback wrapper that filters own patterns on the fly
 *
 * The prototype is the same as the other output_callbacks, using the structure
 * filter_data to store the filter array, the suffix array and the next callback
 *
 */

void own_filter_callback(uint l, uint i, uint n, void* fdata);

// void common_filter_callback(uint l, uint i, uint n, void* fdata);

/**
 * Maximum Common Length
 *
 * Sets the maximum length of a substring at each position i of string s that is
 * contained in the string t.
 *
 * Parameters:
 * r: suffix array of st (concatenation of s and t) 
 * h: lcp of r
 * n: length of st, r and h
 * sn: length of rs, s and m
 * m: output - maximum common lengths for each position in s
 */

void mcl(uint* r, uint* h, uint n, uint* m, uint sn);
void mcl_reverse(uint* r, uint* h, uint n, uint* m, uint sn);

/**
 * Common Substrings Update
 *
 * Update the maximum common length array keeping always the minimum
 *
 * Parameters:
 * m: maximum common length array to update
 * mt: newly calculated maximum common length array
 * n: length of the arrays
 */

void csu(uint* m, uint* mt, uint n);

/**
 * Own Patterns Update
 *
 * Update the maximum common length array keeping always the maximum
 *
 * Parameters:
 * m: maximum common length array to update
 * mt: newly calculated maximum common length array
 * n: length of the arrays
 */

void opu(uint* m, uint* mt, uint n);

/**
 * Common Substrings
 * 
 * Find the common substrings of s and other strings using the already
 * calculated mcl array
 * 
 * Parameters:
 * s: original string
 * h: lcp of s
 * n: length of s
 * ml: minimum length of the substrings
 * r: suffix array of s
 * m: mcl of s and the other strings
 */

void common_substrings(uchar *s, uint n, uint* r, uint* m, uint *h,
	uint ml, output_callback* out, void* data);

#endif // __COP_H__
