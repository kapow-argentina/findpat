#ifndef __KLCP_H__
#define __KLCP_H__

#include "tipos.h"

/* Computes the K function defined as 1+\sum_{i=0}^{n-2}\frac{1}{h[i]+1}
 * where h[i] is the LCP of a given string. Please note that the last
 * value of h (h[n-1]) is never used and assumed as 0.
 * Destroys the values of h.
 */
long double klcp(uint n, uint* h);

/* Computes the same K function as klcp but in a more straightforward
 * way which COULD have a worse precision.
 */
long double klcp_lin(uint n, const uint* h);

/** LCP-Set encode **/
/* A LCP-SET is the set of numbers of the LCP. They could be stored in
 * two ways:
 *  - As an array of (n-1) elements of uint stored in ASCENDING order, or
 *  - As a bitarray (uchar*) of (n-1) bits rounded up to the nearest byte
 *    storing 1 bit for each number meaning 1 if and only if h[i] = h[i-1]+1
 *    assuming h[-1]=0.
 * 
 * These two functions converts between the two representations.
 */
void lcp_settoarr(uchar* hset, uint* h, uint n);
void lcp_arrtoset(uint* h, uchar* hset, uint n);
void lcp_arrtoset_sort(uint* h, uchar* hset, uint n);


typedef long double (*k_ind_func)(int,int);


/* Computes the K function defined as 1+\sum_i f(i,h[i])
 * where h[i] is the LCP of a given string. Please note that the last
 * LCP values are ordered increasingle, so h[i] is actualy the value number
 * i of h, in increasing order
 * Destroys the values of h.
 */
long double klcp_f(uint n, uint* h, k_ind_func f);
/* The same as klcp_f, but over a LCP-SET */
long double klcpset_f(uint n, uchar* hset, k_ind_func f);


/* Computes a fixed set of KLCP_FUNCS functions */
#define KLCP_FUNCS 6
void klcpset_f_arr(uint n, uchar* hset, long double* res);

/*Inverse function, simply returns 1/(v+1) */ 
long double klcp_inverse(int i, int v);

/* Substract lower bound. Substracts to v the minimum possible value and then
 * use the inverse to take it to (0,1]*/ 
long double sub_lb(int i, int v);

/* Normalize. Substract lower bound and normalizes uniformly to (0,1] based
 * on the upper bound */ 
long double klcp_normalize(int i, int v);

long double klcp_bruijn(int i, int v);

long double klcp_inverse2(int i, int v);
long double klcp_normalize2(int i, int v);
long double klcp_bruijn2(int i, int v);

#endif
