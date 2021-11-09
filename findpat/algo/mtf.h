#ifndef __MTF_H__
#define __MTF_H__
#include "tipos.h"

/** mtf_bf: Move-To-Front Fuerza-Bruta
 * Implementa MTF manteniendo la lista de elementos en un arreglo físico.
 * Es la implementación mas simple y corre en O(N*256) (en realidad corre
 * en tiempo O(N + sumatoria(output_i)). La implementación es simple.
 */
void mtf_bf(uchar* src, uchar* dst, uint size);

/** imtf_bf: Inverse Move-To-Front Fuerza-Bruta
 * Misma idea que mtf_bf. Es sólo una simulación del algoritmo sobre un
 * arreglo estático.
 */
void imtf_bf(uchar* src, uchar* dst, uint size);

/** mtf_sqr: Move-To-Front sobre listas enlazadas de longitud sqrt.
 * Implementa la lista del algoritmo como 16 listas enlazadas de longitud
 * 16. De este modo, uno puedo ubicar un elemento en qué lista está, y
 * luego buscar el elemento linealmente dentro de la lista. Esto
 * provee un orden de complejidad de O(N*sqrt(256)). El overhead
 * de las listas enlazadas hace que la constante sea aún grande
 * pero funciona bien en caso promedio.
 */
void mtf_sqr(uchar* src, uchar* dst, uint size);

/** mtf_log: Move-To-Front sobre conjunto de elementos.
 * Implementa la inversa de la lista del algorimo sobre un conjunto
 * indicando la última iteración en la que se usó un caracter dado. La
 * posición entonces se interpreta como cuántos elementos hay en el conjunto
 * mayores que su última aparición. El conjunto se implementa sobre
 * arreglo de bool de 0 a X con una estructura de árbol sobre arreglo
 * para garantizar tiempo log en la consulta cuántos mayores que "i" hay
 * en el conjunto. Cuando los números llegan a X, se comprimen todos los
 * números al rango 0 a 255 y se continúa con la misma implementación.
 * Provee un orden O(N * log(X) + X*N/(X-256)). En este caso X = 2048.
 * El overhead es considerablemente bajo pues la estructura es muy simple.
 * returns the number of 0 in the output
 */
uint mtf_log(uchar* src, uchar* dst, uint size);

#endif
