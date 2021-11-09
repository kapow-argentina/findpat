#ifndef __SA_SEARCH_H__
#define __SA_SEARCH_H__

#include "tipos.h"
#include "output_callbacks.h"

/* Simple algorithm to find all the occurrences of a substring in a given string */

/*
Input:
   	s -> source string
	r -> suffix array of s
	n -> length of s
	pat -> target string
	len -> length of pat

Output:
	The occurrences of <pat> are in the range [*from, *to)
	of the suffix array <r>.

	Returns TRUE iff the range is not empty.

Time complexity: O(|pat| log(|s|))

*/
bool sa_search(uchar *s, uint *r, uint n, uchar *pat, uint len, uint *from, uint *to);


/* Manber and Myers (1993) algorithm to find a substring in a given string
 *
 * It uses two data structures called llcp and rlcp with the following definition
 *
 *   llcp[m] = LCP( s[ r[l_m] .. n ],  s[ r[m  ] .. n ] )
 *   rlcp[m] = LCP( s[ r[m  ] .. n ],  s[ r[r_m] .. n ] )
 *
 * where (l_m, m, r_m) is the only possible triple with the given m in the
 * binary search algorithm. See the paper for futher reference.
 *
 */

/** build_lrlcp
 * Builds llcp and rlcp given the lcp array(h)
 * The lcp can be the same array as the llcp or rlcp, as it stored just after being used */
void build_lrlcp_8(uint n, uchar* h, uchar* llcp, uchar* rlcp);
void build_lrlcp_16(uint n, ushort* h, ushort* llcp, ushort* rlcp);
void build_lrlcp_32(uint n, uint* h, uint* llcp, uint* rlcp);
#define build_lrlcp build_lrlcp_32

void build_lrlcp_lk_8(uint n, uchar* h, uchar* llcp, uchar* rlcp, uint lookup, uint* lk);
void build_lrlcp_lk_16(uint n, ushort* h, ushort* llcp, ushort* rlcp, uint lookup, uint* lk);
void build_lrlcp_lk_32(uint n, uint* h, uint* llcp, uint* rlcp, uint lookup, uint* lk);
#define build_lrlcp_lk build_lrlcp_lk_32

/** sa_mm_search

Time complexity: O(|pat| + log(|s|))

*/

bool sa_mm_search_8(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const uchar* llcp, const uchar* rlcp);
bool sa_mm_search_16(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const ushort* llcp, const ushort* rlcp);
bool sa_mm_search_32(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const uint* llcp, const uint* rlcp);
#define sa_mm_search sa_mm_search_32

bool sa_mm_search_dna_8(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const uchar* llcp, const uchar* rlcp, uint lookup, uint* lk);
bool sa_mm_search_dna_16(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const ushort* llcp, const ushort* rlcp, uint lookup, uint* lk);
bool sa_mm_search_dna_32(uchar *s, const uint *r, const uint n, uchar *pat, uint len, uint *from, uint *to, const uint* llcp, const uint* rlcp, uint lookup, uint* lk);

#endif /* __SA_SEARCH_H__ */

