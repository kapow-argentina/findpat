#include "rle.h"

/** Implementaci'on del RLE:
 * 3 o menos caracteres seguidos se copian tal cual
 * 4 o más se codifican así:
 *  XXXX<n>  donde n es la cantidad de caracteres X -4 (un byte).
 * si <n> sobrepasa 255 se pone un 255 y se continua asumiendo que XXXX<255> cuenta como XXX en lo que sige.
 * ejemplo 280 X se codifica en XXXX<255>X<20>   280 = 4 + 255 + 1 + 20
 */

#include "macros.h"

uint rle(uchar* src, uchar* dst, uint n) {
	uint lx = 256;
	uint m=0, c=0;
	for(;n--; ++src) {
		if (*src == lx) {
			c++;
			if (c == 255+4+1) ++m, *(dst++)=255, c=4;
		} else {
			if (c >= 4) ++m, *(dst++)=c-4;
			lx = *src; c = 1;
		}
		if (c <= 4) ++m,*(dst++) = lx;
	}
	if (c >= 4) ++m, *(dst++)=c-4;
	return m;
}

uint rle_len(uchar* src, uint n) {
	uint lx = 256;
	uint m=0, c=0;
	for(;n--; ++src) {
		if (*src == lx) {
			c++;
			if (c == 255+4+1) ++m, c=4;
		} else {
			if (c >= 4) ++m;
			lx = *src; c = 1;
		}
		if (c <= 4) ++m;
	}
	if (c >= 4) ++m;
	return m;
}

uint irle(uchar* src, uchar* dst, uint n) {
	uint lx = 256;
	uint m=0, c=0;
	for(;n--; ++src) {
		++m, *(dst++) = *src;
		if (*src == lx) {
			c++;
			if (c == 4 && n) { n--; ++src;
				c = *src;
				while (c--) ++m, *(dst++) = lx;
				c = 3;
			}
		} else {
			c = 1; lx = *src;
		}
	}
	return m;
}

uint irle_len(uchar* src, uint n) {
	uint lx = 256;
	uint m=0, c=0;
	for(;n--; ++src) {
		++m;
		if (*src == lx) {
			c++;
			if (c == 4 && n) { n--; ++src;
				m += *src; c = 3;
			}
		} else {
			c = 1; lx = *src;
		}
	}
	return m;
}
