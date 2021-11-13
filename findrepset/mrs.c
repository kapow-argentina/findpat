#include "mrs.h"
#include <stdio.h>
#include <stdlib.h>

#include "bittree.h"
#include "sorters.h"

#include "macros.h"
#include "output_callbacks.h"

static __thread uint* data;
#define DATA_VAL(x) data[*(x)]
_def_qsort3(qsort_data, uint, uint, DATA_VAL, <)

void sort(uint n, uint* _data, uint* ind) {
	uint i;
	data = _data;
	forn(i,n) ind[i] = i;
	qsort_data(ind, ind+n);
}

void mrs(uchar* s, uint n, uint* r, uint* h, uint* p, uint ml,
		 output_callback out, void* data) {
	
	uint i,ii,j,k,n1=n+1;
	/*TODO: trick to use half the memory for ind*/
	uint* ind = (uint*)pz_malloc((n-1) * sizeof(uint));
	bittree* tree = bittree_malloc(n1);
	bittree_clear(tree, n1);
	bittree_preset(tree,n1,0);
	bittree_preset(tree,n1,n);
	forn(i,n-1) if (h[i] < ml) bittree_preset(tree,n1,i+1);
	bittree_endset(tree,n1);

	sort(n-1, h, p);
//	fprintf(stderr, "mrs: sort end\n");
//	DBGun(p, n);

	forn(i,n-1) ind[i] = p[i];
	forn(i,n) p[r[i]] = i;

	forn(ii,n-1) {
		i = ind[ii];
		/*showtree(tree, n1);
		printf("%d (%d)\n", i, h[i]);*/
		if (h[i] < ml) continue;
		j = bittree_max_less_than(tree, n1, i+1);

		if (j > 0 && h[j-1]==h[i]) {
			bittree_set(tree, n1, i+1);
			continue;
		}
		k = bittree_min_greater_than(tree, n1, i+1) - 1;
		bittree_set(tree, n1, i+1);

		if (k < n-1 && h[k]==h[i]) continue;
		if (r[j] > 0 && r[k] > 0 && s[r[j]-1] == s[r[k]-1]
			&& p[r[k]-1]-p[r[j]-1]==k-j) continue;
		out(h[i], j , k-j+1, data);
	}
	bittree_free(tree, n1);
	pz_free(ind);
}

