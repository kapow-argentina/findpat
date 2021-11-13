#include "bittree.h"

#include <stdlib.h>
#include <assert.h>

#include "tipos.h"
#include "macros.h"

#define IS_LEFT_CHILD(i) ((~i)&1)
#define IS_RIGHT_CHILD(i) (i&1)
#define LEFT_CHILD(i) (i<<1)
#define RIGHT_CHILD(i) ((i<<1)|1)
#define PARENT(i) (i>>1)

bittree* bittree_malloc(uint n) {
	uint i, logn = 1;
	for(i=1;i<n;i<<=1) ++logn;
	bittree* tree = (bittree*)pz_malloc(logn * sizeof(word_type*));
	forn(i,logn) {
		tree[i]=bita_malloc(n);
		n = (n>>1)+(n&1);
	}
	assert(n==1);
	return tree;
}

void bittree_free(bittree* tree, uint n) {
	uint i, logn = 1;
	for(i=1;i<n;i*=2) ++logn;
	forn(i,logn) pz_free(tree[i]);
	pz_free(tree);
}

void bittree_clear(bittree* tree, uint n) {
	uint i, j, logn = 1;
	for(i=1;i<n;i<<=1) ++logn;
	forn(i,logn) {
		forn(j,(n+ba_word_size-1)/ba_word_size) tree[i][j]=0;
		n = (n>>1)+(n&1);
	}
	assert(n==1);
}

void bittree_preset(bittree* tree, uint n, uint i) {
	bita_set(tree[0],i);
}

void bittree_endset(bittree* tree, uint n) {
	uint i, j, logn = 1;
	for(i=1;i<n;i<<=1) ++logn;
	forn(i,logn-1) {
		forn(j,n) if (bita_get(tree[i],j)) {
			bita_set(tree[i+1],PARENT(j));
		}
		n = (n>>1)+(n&1);
	}
}

void bittree_set(bittree* tree, uint n, uint i) {
	uint j, logn = 1;
	for(j=1;j<n;j<<=1) ++logn;
	bita_set(tree[0],i);
	forsn(j,1,logn) {
		i>>=1;
		if (bita_get(tree[j],i)) break;
		bita_set(tree[j],i);
	}
}


uint bittree_max_less_than(bittree* tree, uint n, uint i) {
	assert(0 <= (i) && (i) < n);
//	assert(!bita_get(tree[0],i));
	uint l = 0;
	while (!bita_get(tree[l],i)) {
		if (IS_LEFT_CHILD(i)) {
			i = i-1;
		} else {
			i = PARENT(i);
			++l;
		}
	}
	while(l>0) {
		if (bita_get(tree[l-1],RIGHT_CHILD(i))) {
			i = RIGHT_CHILD(i);
		} else {
			i = LEFT_CHILD(i);
		}
		--l;
	}
	return i;
}

uint bittree_min_greater_than(bittree* tree, uint n, uint i) {
	assert(0 <= (i) && (i) < n);
//	assert(!bita_get(tree[0],i));
	uint l = 0;
	while (!bita_get(tree[l],i)) {
		if (IS_RIGHT_CHILD(i)) {
			i = i+1;
		} else {
			i = PARENT(i);
			++l;
		}
	}
	while(l>0) {
		if (bita_get(tree[l-1],LEFT_CHILD(i))) {
			i = LEFT_CHILD(i);
		} else {
			i = RIGHT_CHILD(i);
		}
		--l;
	}
	return i;
}


void bittree_show(bittree* tree, uint n) {
	uint i,j,k,sp = 1,logn = 1;
	for(j=1;j<n;j<<=1) ++logn;
	forn(k,logn) {
		forn(j,sp/2) printf(" ");
		forn(i,n) {
			printf("%d", bita_get(tree[k],i));
			forn(j,sp) printf(" ");
		}
		printf("\n");
		sp = 2*sp+1;
		n = (n>>1)+(n&1);
	}
}
