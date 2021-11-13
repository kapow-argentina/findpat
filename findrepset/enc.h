#ifndef __ENC_H__
#define __ENC_H__

#include "tipos.h"

/**
 * Encodea un stream FASTA de 5 letras en un alfabeto de 250 caracteres.
 * Quita el header de los archivos FASTA en caso de encontrarlo y devuelve
 * la longitud del resultado (long_original - FASTA header).
 */
uint fa_encode(uchar* src, uchar* dst, uint n);

/**
 * Inversa de fa_encode. No restituye el header, obviamente
 */
void fa_decode(uchar* src, uchar* dst, uint n);


/**
 * Quita el header FASTA del inicio y devuelve el puntero al comienzo
 * de los datos en crudo.
 */
uchar* fa_strip_header(uchar* src, uint n);

uint fa_copy_cont(uchar* src, uchar* dst, uint n);

uint fa_strip_n(uchar* fasrc, uchar* dst, uint n);
uint fa_strip_n_and_blanks(uchar* src, uchar* dst, uint n);

uint fa_strip_n_trac(uchar* src, uchar* dst, uint n, uint **trac_buf, uint *trac_size, uint trac_middle);

uint trac_convert_pos_virtual_to_real(uint x, uint* trac_buf, uint trac_size);
uint trac_convert_pos_real_to_virtual(uint x, uint* trac_buf, uint trac_size);

#endif
