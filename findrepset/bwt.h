#ifndef __BWT_H__
#define __BWT_H__

#include "tipos.h"

/** bwt() toma la cadena s de largo n (utilizando
 * los primeros n bytes de p si src==NULL, o src en caso contrario) y
 * un arreglo de enteros r de largo n y deja en r
 * una permutacion tal que la cadena
 * s[i] s[i+1] ... s[n] s[0] ... s[i-1] esta en la
 * posicion r[i] en el orden de rotaciones
 * p tiene que tener lugar para n enteros
 *
 * Output:
 *  bwt is the output string (src with the r permutation aplyed).
 *  src it the original string.
 *  r should be the lexicographical order of all rotations of src
 *  p[i] will be the rank of the rotation i (if not overlap with src or bwt)
 *
 * src and bwt could be both NULL. See bwt_src_bc() below for details.
 */

void bwt(uchar *bwt, uint* p, uint* r, uchar* src, uint n, uint* prim);

/** obwt() toma la cadena s de largo n (utilizando
 * los primeros n bytes de p si src==NULL, o src en caso contrario) y
 * un arreglo de enteros r de largo n y deja en r
 * una permutacion tal que la cadena
 * s[i] s[i+1] ... s[n] s[0] ... s[i-1] esta en la
 * posicion r[i] en el orden de rotaciones
 * p tiene que tener lugar para n enteros
 *
 * Output:
 *  bwt is the output string (src with the r permutation aplyed).
 *  src it the original string.
 *  r should be the lexicographical order of all rotations of src
 *  p[i] will be the rank of the rotation i (if not overlap with src or bwt)
 *
 * src and bwt could be both NULL. See bwt_src_bc() below for details.
 */


void obwt(uchar *bwt, uint* p, uint* r, uchar* src, uint n, uint* prim);

/**
 * Inverse of bwt. src != dst.
 * Uses an internal array of uint of length n.
 */
void ibwt(uchar *src, uchar *dst, uint n, uint prim);

/**
 * bwt to src-p-r vectors.
 * Takes as input an uchar array of length n (bwt)
 * wich is the bwt output of some stream (src), the index
 * of the position of the orginal circular stream in the bwt matrix (called prim)
 * and the length n.
 *
 * As output,
 *  p[i] will be the rank of the rotation i
 *  r should be the lexicographical order of all rotations of src
 *  src will be the original string.
 *
 * The array bwt and src could be overlaped with (or included in) p,
 *  BUT must not overlap each other.
 *  If bwt is NULL, point to the first n uchars of array p is asumed.
 *  If src is NULL, point to the last n uchars of array p is asumed.
 * See bwt_src_bc().
 */
void bwt_spr(uchar *bwt, uint *p, uint *r, uchar *src, uint n, uint prim);

/**
 * r & bc to bwt_out
 *
 * Input:
 *  bc is a 256 length vector with the amount of occurrences of each character.
 *  r should be the lexicographical order of all rotations of the original string src
 *
 * Output:
 *  bwt is the output string (src with the r permutation aplyed).
 *  src it the original string.
 *  bc  is destroyed
 *  r is unchanged
 *
 * The array bwt and src could be overlaped with (or included in) p,
 *  BUT must not overlap each other. The array p is unused, unless it overlap src or bwt.
 *  If bwt is NULL, point to the first n uchars of array p is asumed.
 *  If src is NULL, point to the last n uchars of array p is asumed.
 *  If bwt and src ar both non-NULL, p could be safely NULL.
 */
void bwt_src_bc(uchar *bwt, uint *p, uint *r, uchar *src, uint n, uint* bc);

/**
 * r & p & bc to s & bwt_out
 *
 */
void bwt_rsrc_pbc(uchar *bwt, uint *p, uint *r, uchar* src, uint n, uint* bc);

void bwt_build_bc(uchar* src, uint n, uint* bc);

#endif //__BWT_H__
