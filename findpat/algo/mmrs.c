#include "mmrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "output_callbacks.h"

//static __thread uint* data;
#define DATA_VAL(x) data[*(x)]

void mmrs(uchar* s, uint n, uint* r, uint* h, uint ml,
		 output_callback out, void* data) {
	uint i,j,k,up = 0;
	uchar prev;
	uint alph_size = 1 << sizeof(uchar) * 8;
	bool* alph = (bool*)pz_malloc(alph_size * sizeof(bool));
	bool coll;
//	h[n-1] = 0;
	memset(alph, 0, alph_size * sizeof(bool));

	forsn(i, 1, n-1) {
		
		/* mark the last step up */
		if (h[i] > h[i-1]) {
			up = i;
			continue;
		}

		if (h[i] == h[i-1]) continue;
		
		/* now we're going downhill */
		if (up != -1 && h[i-1] >= ml){
			coll = 0;

			forsn(j, up, i+1){
				if (r[j] > 0){
					prev = s[r[j]-1];
					if (alph[prev]){
						coll = 1;
						break;
					}
					alph[prev] = 1;
				}
			}
			if (coll == 0) out(h[up], up , i-up+1, data);
			forsn(k, up, j) if (r[k] > 0) alph[s[r[k]-1]] = 0;
			/* warning: setting an unsigned int with a negative value */
			up = -1; 
		}
	}
	pz_free(alph);
}

