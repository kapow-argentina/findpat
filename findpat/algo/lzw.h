#ifndef __LZW_H__
#define __LZW_H__

#include "tipos.h"

#define LZW_DICTSIZE (1<<14)

/**
 * Comprime el flujo de datos uchar*src usando LZW y un diccionario
 * con eliminación LRU sobre un diccionario de LZW_DICTSIZE códigos > 257.
 * La salida se escribe en códigos en dst.
 */
uint lzw(uchar* src, ushort* dst, uint size);

/**
 * Descomprime el flujo de datos de ushort*src usando LZW y devuelve su
 * longitud. Si dst es NULL sólo calcula su longitud y no escribe la
 * salida descomprimida en dst.
 */
uint ilzw(ushort* src, uchar* dst, uint size);

/**
 * Descomprime el flujo de datos de ushort*src usando LZW y devuelve su
 * longitud. No escribe la salida descomprimida.
 */
uint ilzw_len(ushort* src, uint size);

#endif
