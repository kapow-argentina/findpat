#include "cop.h"
#include "macros.h"

/* Maximum Common Length  */

/* Two strings are involved, <s> and <t>
 * 
 * <r> could be
 *    (1) the suffix array of <s> + <t>
 * or (2) the suffix array of <t> + <s>
 *
 * We define two versions of Maximum Common Length,
 *   mcl             for (1)
 *   mcl_reverse     for (2)
 */
#define define_mcl(NAME, IS_IN_S, R) 	\
/* TODO: in the case of own patterns it is possible to update the array in-place */ \
void NAME(uint* r, uint* h, uint n, uint* m, uint sn){ \
	/* indices */ \
	uint i; \
	/* current maximum common characters */ \
	uint cm = 0; \
	/* current index is in s */ \
	bool cs = IS_IN_S(0); \
	if (cs) m[R(0)] = 0; \
	/* Possible TODO: update upwards and downwards simultaneously */ \
	/* update downwards */  \
	forn(i, n-1){ \
		/* tt & st*/ \
		if(!IS_IN_S(i + 1)){ \
			cs = FALSE; \
			continue; \
		}	 \
		/* ts */ \
		if (!cs){  \
			cm = h[i]; \
			cs = TRUE; \
		} \
		/* ss */ \
		else if (h[i] < cm) cm = h[i]; \
		m[R(i+1)] = cm; \
	} \
	cs = IS_IN_S(n-1); \
	cm = 0; \
	/* update upwards */  \
	for(i = n-2; i != -1; i--){ \
		/* tt & st*/ \
		if(!IS_IN_S(i)){ \
			cs = FALSE; \
			continue; \
		}	 \
		/* ts */ \
		if (!cs){  \
			cm = h[i]; \
			cs = TRUE; \
		} \
		/* ss */ \
		else if (h[i] < cm) cm = h[i]; \
		if (cm > m[R(i)]) m[R(i)] = cm; \
	} \
}

/* Define the two versions of mcl */

#define is_in_s(i) 		(r[(i)] < sn)
#define r_at(i)			(r[(i)])
define_mcl(mcl, is_in_s, r_at)
#undef is_in_s
#undef r_at

#define is_in_s_reverse(i) 	(r[(i)] >= n - sn)
#define r_at_reverse(i) 	(r[(i)] - n + sn)
define_mcl(mcl_reverse, is_in_s_reverse, r_at_reverse)
#undef is_in_s_reverse
#undef r_at_reverse

void csu(uint* m, uint* mt, uint n){
	uint i;
	forn(i,n) if(mt[i] < m[i]) m[i] = mt[i];
}

void opu(uint* m, uint* mt, uint n){
	uint i;
	forn(i,n) if(mt[i] > m[i]) m[i] = mt[i];
}


void own_filter_callback(uint l, uint i, uint n, void* fdata){
	int j = ((filter_data*)fdata)->r[i];
	if ( l > ((filter_data*)fdata)->filter[j])
		((filter_data*)fdata)->callback(l, i, n, ((filter_data*)fdata)->data);
}

void common_substrings(uchar *s, uint n, uint* r, uint* m, uint *h, uint ml, output_callback* out, void* data){

	uint i;
	bool alive = TRUE;
	forn(i, n-1){
		// maximality to the left
		if (alive && r[i] > 0 && m[r[i]-1] > m[r[i]]) alive = FALSE;
		//printf("i %d, s[r[i]] %c, r[i] %d, alive %d, ml %d, h[i] %d, m[r[i]] %d\n", i, s[r[i]], r[i], alive, ml, h[i], m[r[i]]);
		
		if (h[i] >= m[r[i]]) {
			if (m[r[i+1]] > m[r[i]]) alive = TRUE;
		} else {
			if (alive && (m[r[i]] >= ml)) {
				//printf("l %d i %d r[i] %d\n", m[r[i]], i, m[r[i]]);
				out(m[r[i]], i, 1, data);
			}
			alive = (m[r[i]] < m[r[i+1]] || h[i] < m[r[i+1]]);
		}
	}
	
	if (alive && (r[i] > 0 && m[r[i]-1] > m[r[i]]) && (m[r[i]] >= ml)) 
		out(m[r[i]], i, 1, data);
}

