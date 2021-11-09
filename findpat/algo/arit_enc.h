#ifndef __ARIT_ENC_H__
#define __ARIT_ENC_H__

#include "tipos.h"

/* Runs arithmetic encoding on the string src of length n and returns the
 * result on the string dst (the size of dst is returned). Beware to allocate
 * enough space on dst.
 * TODO: Como hacemos con la interfase si no sabemos cuanto va a ocupar?
 */
uint arit_enc(ushort* src, uint n, uchar* dst);

/* Inverse of arit_enc. n is the length of src. The length of
 * src is returned
 */
uint arit_dec(ushort* src, uint n, uchar* dst);

/* Estimates the size of the arithmetic encoding of the string src
 * of length n
 */
uint arit_enc_est(ushort* src, uint n);

/* Calculates the entropy of the given string
 */
double entropy_char(uchar* src, uint n);
double entropy_short(ushort* src, uint n);

#endif //__ARIT_ENC_H__
