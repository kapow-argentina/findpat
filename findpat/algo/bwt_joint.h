#ifndef __BWT_JOINT_H__
#define __BWT_JOINT_H__

#include "tipos.h"



/** bwt_joint() toma la cadena s de largo n y t de largo m
 * (que no deben contener el caracter 255)
 * y un arreglo de enteros r de largo n+m y deja en r
 * una permutacion tal que la cadena
 * u[i] u[i+1] ... u[n+m] u[0] ... u[i-1] esta en la
 * posicion r[i] en el orden de rotaciones de u=st
 * p tiene que tener lugar para n enteros
 *
 * Al terminar deja la permutación que ordena la entrada en r
 * y deja al comienzo la última columna de la BWT.
 */
void bwt_joint(uchar* s, uint n, uchar* t, uint m, uint* r, uint* p, 
               uint* prim);

/**
  * Dadas dos cadenas, deja r y p preparados para llamar a bwt_joint_hint
  */
void bwt_joint_prepare(uchar* s, uint n, uchar* t, uint m, uint* r, uint* p);

/** Hace lo mismo que la anterior solo que se asegura que en r esta la bwt de
  * s$ (quitando la rotacion que empieza de $) y a continuacion la bwt de t$
  * (quitando la rotacion que empieza de $)
  * Las cadenas de entrada se pasan usando los primeros n+m bytes de p (en los
  * primeros n va s y en los siguientes m va t)
  */
void bwt_joint_hint(uint* p, uint* r, uint n, uint m, uint* prim);

#endif //__BWT_JOINT_H__
