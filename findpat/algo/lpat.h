#ifndef __LPAT_H__
#define __LPAT_H__

#include "tipos.h"

/*
 * Calculates the length of the longest pattern (maximal repeated substring)
 * that overlaps each position.
 *
 * Input:
 *   n:    size of the input string
 *   s:    input string
 *   r:    suffix array of the input string
 *   p:    inverse permutation of r
 *
 * Output:
 *   p[i] = length of the longest pattern that overlaps the ith position of the string
 *
 *   Overwrites r.
 *
 */
void longest_mrs(uint n, uchar *s, uint *r, uint *p);

/* Similar to longest_mrs, but only for patterns that occur in both strings. */
void longest_mrs_full2(uint n, uint long_input_a, uchar *s, uint *r, uint *p);

#endif /* __LPAT_H__ */
