#ifndef __RLE_H__
#define __RLE_H__
#include "tipos.h"

/** rle: Run-Lenght enconding
 * Implementa RLE sobre el arreglo de char y pone la salida en DST.
 * Devuelve la cantidad de bytes escritos en la salida.
 * OJO: La salida puede ser un 20% más grande (ver rle_len())
 */
uint rle(uchar* src, uchar* dst, uint size);

/** rle_len: Run-Lenght enconding lenght
 * Calcula la longitud de la salida del RLE.
 */
uint rle_len(uchar* src, uint size);

/** irle: Inverse Run-Lenght enconding
 * Implementa la inversa de RLE sobre el arreglo de char y pone la salida en DST.
 * Devuelve la cantidad de bytes escritos en la salida.
 * OJO: La salida puede ser MUCHO más grande (ver irle_len())
 */
uint irle(uchar* src, uchar* dst, uint size);

/** rle_len: Run-Lenght enconding lenght
 * Calcula la longitud de la salida del RLE.
 */
uint irle_len(uchar* src, uint size);


#endif
