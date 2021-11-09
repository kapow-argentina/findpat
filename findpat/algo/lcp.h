#ifndef __LCP_H__
#define __LCP_H__

#include "tipos.h"

/* Takes as input 2 uint arrays of length at least n (p and r)
 * and the original string of length n (s)
 * r should be the lexicographical order of all rotations of s
 * p should be the inverse permutation of r
 * the output is given on r
 */
void lcp(uint n, const uchar* s, uint* r, uint* p);


/**
 * lcp3 and lcp3_sp do the same as lcp, but the output is given on h.
 * Nevertheless, h and r could be the same pointer.
 * lcp3_sp considers the char sp to be diferent to itself.
 */
void lcp3(uint n, const uchar* s, const uint* r, uint* p, uint* h);
void lcp3_sp(uint n, const uchar* s, const uint* r, uint* p, uint* h, unsigned char sp);


/**
 * lcp_mem3 and lcp_mem3_sp compute the LCP in a uchar* vector h, using
 * a temporary array p of size n/4, consuming a peak of n extra bytes.
 */
void lcp_mem3(uint n, const uchar* s, const uint* r, uchar* h);
void lcp_mem3_sp(uint n, const uchar* s, const uint* r, uchar* h, unsigned char sp);

#endif //__LCP_H__
