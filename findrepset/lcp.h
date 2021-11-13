#ifndef __LCP_H__
#define __LCP_H__

#include "tipos.h"

/* Takes as input 2 uint arrays of length at least n (p and r)
 * and the original string of length n (s)
 * r should be the lexicographical order of all rotations of s
 * p should be the inverse permutation of r
 * the output is given on r
 */
void lcp(uint n, uchar* s, uint* r, uint* p);

#endif //__LCP_H__
