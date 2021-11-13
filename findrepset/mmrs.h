#ifndef __MMRS_H__
#define __MMRS_H__

#include "tipos.h"
#include <stdio.h>
#include "output_callbacks.h"

/**
 * Calculates the maximal maximal repeated substrings of the input string s of
 * size n.  r, and h must be of size n.
 * r must contain the starting point of all suffixes of s in lexicographical
 * order. h[i] must contain the length of the longest common prefix between
 * s[r[i]]s[r[i]+1]...s[n] and s[r[i+1]]s[r[i+1]+1]...s[n]. 
 * ml is the minimum length a substring has to have to be considered
 * The output is given by calling out with the extra parameter data 
 * (see above).
 */
void mmrs(uchar* s, uint n, uint* r, uint* h, uint ml,
		 output_callback out, void* data);

#endif // __MMRS_H__
