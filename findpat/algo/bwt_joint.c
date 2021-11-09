#include "bwt_joint.h"
#include "bwt.h"
#include "macros.h"

#include <stdlib.h>
#include <string.h>

void bwt_joint(uchar* s, uint n, uchar* t, uint m, uint* r, uint* p,
               uint* prim) {
	bwt_joint_prepare(s, n, t, m, r, p);
	bwt_joint_hint(p, r, n, m, prim);
}

void bwt_joint_prepare(uchar* s, uint n, uchar* t, uint m, uint* r, uint* p) {
	uint* rt;
	memcpy(p, s, n);
	((uchar*)p)[n]=255;
	bwt(NULL, p, r, NULL, n+1, NULL);
	rt = (uint*)pz_malloc((m+1) * sizeof(uint));
	memcpy(p, t, m);
	((uchar*)p)[m]=255;
	bwt(NULL, p, rt, NULL, m+1, NULL);
	memcpy(r+n+1, rt, (m+1)*sizeof(uint));
	pz_free(rt);
	memcpy(p, s, n);
	((uchar*)p)[n]=255;
	memcpy(((uchar*)p)+n+1, t, m);
	((uchar*)p)[n+m+1]=255;
}

#define SHORTNUM (256*256)
#define CONCAT(_F,_S) ((((ushort)_F) << 8) | ((ushort)_S))
void bwt_joint_hint(uint* p, uint* r, uint n, uint m, uint* prim) {
	/*TODO: regenerar la entrada en p como el bwt normal.*/
	uint i,j,mid,i1,i2,it,ini,other1,other2,nubi=0,t,nm2=n+m+2;
	ushort cc;
	uchar *s = (uchar*)p;
	uint *mrs, *rs, *rt, *raux, *bc;
	bc = (uint*)pz_malloc(SHORTNUM*sizeof(uint));
	rt = r;
	mrs = rs = (uint*)pz_malloc(nm2 * sizeof(uint)); // mrs saves the pointer for later pz_free()
	forsn(i,n+1,nm2) rt[i] += n+1; //fix hint
	memset(bc, 0, SHORTNUM*sizeof(uint));
	forn(i,nm2-1) ++bc[CONCAT(s[i],s[i+1])];
	++bc[CONCAT(s[nm2-1],s[0])];
	/*Accumulate frecuencies including the given index */
	forsn(i, 1, SHORTNUM) { bc[i]+=bc[i-1]; }
	/* Initialize indices (stable bucket sort)*/
	dforn(i, nm2) {
		j = rt[i];
		if (j != nm2-1) {
			rs[--bc[CONCAT(s[j],s[j+1])]] = j;
		} else {
			rs[--bc[CONCAT(s[j],s[0])]] = j;
		}
	}
	/*This leaves accumulated frecuencies not including the given index*/
	/*Initialize position number according to first 2 characters*/
	cc = CONCAT(s[nm2-1],s[0]);
	p[nm2-1] = ((cc < (256*256-1)) ? bc[cc+1]:nm2) - 1;
	dforn(i, nm2-1) {
		cc = CONCAT(s[i],s[i+1]);
		p[i]=((cc < (256*256-1)) ? bc[cc+1]:nm2) - 1;
	}

	//forn(i,nm2) printf("%u(%u)  ", rs[i], p[rs[i]]); printf("\n");
	for(t = 2; t < nm2; t*=2) {
		//printf("***t=%u***\n", t);
		/*If there are many items in unsorted buffers or the return array
		 *is not the current array, then move everything to the other array.
		 *Ohterwise, only use the other for merge and return to source array*/
		if (nubi > nm2/2 || rt == r) {
			nubi = 0;
			for(i = 0, j = 1; i < nm2; i = j++) {
				/*Calculate next bucket, if it comes entirely from one string, it is already sorted*/
				if (rs[i] >= n+1) { /*Entire bucket comes from first string*/
					//printf("2nd BUCKET %u(%u)<%u>", i, rs[i], p[rs[i]]);
					rt[i]=rs[i];
					while(j < nm2 && p[rs[j]] == p[rs[i]]) p[rt[j]=rs[j]]=j, ++j;
					//printf(" %u(%u)<%u>\n", j-1, rs[j-1], p[rs[j-1]]);
				} else {
					mid = -1;
					while(j < nm2 && p[rs[j]] == p[rs[i]]) {
						if (rs[j] >= n-1) {
							mid = j;
							break;
						}
						++j;
					}
					while(j < nm2 && p[rs[j]] == p[rs[i]]) ++j;
					if (mid == -1) { /*Entire bucket comes from second string*/
						//printf("1st BUCKET %u(%u)<%u> %u(%u)<%u> next=%u\n", i, rs[i], p[rs[i]], j-1, rs[j-1], p[rs[j-1]], p[rs[j]]);
						while(i < j) p[rt[i] = rs[i]]=i, ++i;
					} else { /*Mix bucket, do merge*/
						//printf("MIX BUCKET %u(%u) <%u> %u(%u)\n", i, rs[i], mid, j-1, rs[j-1]);
						i1 = it = i;
						i2 = mid;
						while(i1 < mid && i2 < j) {
							other1 = p[(rs[i1]+t)%nm2];
							other2 = p[(rs[i2]+t)%nm2];
							//printf("%u %u\n", other1, other2);
							if (other1 == other2) {
								rt[it++] = rs[i1++];
								while(i1 < mid && p[(rs[i1]+t)%nm2] == other2) rt[it++] = rs[i1++];
								rt[it++] = rs[i2++];
								while(i2 < j && p[(rs[i2]+t)%nm2] == other1) rt[it++] = rs[i2++];
								nubi += it - i; /*Number of items in unfinished bucket*/
								while(i < it) {
									//if (p[rt[i]] < it - 1) printf("AUCH %u %u %u %u", i, rt[i], p[rt[i]],it), exit(0);
									p[rt[i++]] = it-1; //print every position number
								}
							} else if (other1 < other2) {
								p[rt[it] = rs[i1++]] = it; ++it;
								while(i1 < mid && p[(rs[i1]+t)%nm2] < other2) p[rt[it] = rs[i1++]] = it, ++it;
								i = it;
							} else {
								p[rt[it] = rs[i2++]] = it; ++it;
								while(i2 < j && p[(rs[i2]+t)%nm2] < other1) p[rt[it] = rs[i2++]] = it, ++it;
								i = it;
							}
						}
						while(i1 < mid) p[rt[it] = rs[i1++]] = it, ++it;
						while(i2 < j) p[rt[it] = rs[i2++]] = it, ++it;
					}
				}
			}
			raux = rt; rt = rs; rs = raux;
		} else {
			nubi = 0;
			for(i = 0, j = 1; i < nm2; i = j++) {
				/*Calculate next bucket, if it comes entirely from one string, it is already sorted*/
				if (rs[i] >= n+1) { /*Entire bucket comes from first string*/
					//printf("2nd BUCKET %u(%u)<%u>", i, rs[i], p[rs[i]]);
					while(j < nm2 && p[rs[j]] == p[rs[i]]) p[rs[j]]=j, ++j;
					//printf(" %u(%u)<%u>\n", j-1, rs[j-1], p[rs[j-1]]);
				} else {
					mid = -1;
					while(j < nm2 && p[rs[j]] == p[rs[i]]) {
						if (rs[j] >= n-1) {
							mid = j;
							break;
						}
						++j;
					}
					while(j < nm2 && p[rs[j]] == p[rs[i]]) ++j;
					if (mid == -1) { /*Entire bucket comes from second string*/
						//printf("1st BUCKET %u(%u)<%u> %u(%u)<%u> next=%u\n", i, rs[i], p[rs[i]], j-1, rs[j-1], p[rs[j-1]], p[rs[j]]);
						while(i < j) p[rs[i]]=i, ++i;
					} else { /*Mix bucket, do merge*/
						//printf("MIX BUCKET %u(%u) <%u> %u(%u)\n", i, rs[i], mid, j-1, rs[j-1]);
						i1 = it = ini = i;
						i2 = mid;
						while(i1 < mid && i2 < j) {
							other1 = p[(rs[i1]+t)%nm2];
							other2 = p[(rs[i2]+t)%nm2];
							//printf("%u %u\n", other1, other2);
							if (other1 == other2) {
								rt[it++] = rs[i1++];
								while(i1 < mid && p[(rs[i1]+t)%nm2] == other2) rt[it++] = rs[i1++];
								rt[it++] = rs[i2++];
								while(i2 < j && p[(rs[i2]+t)%nm2] == other1) rt[it++] = rs[i2++];
								nubi += it - i; /*Number of items in unfinished bucket*/
								while(i < it) {
									//if (p[rt[i]] < it - 1) printf("AUCH %u %u %u %u", i, rt[i], p[rt[i]],it), exit(0);
									p[rt[i++]] = it-1; //print every position number
								}
							} else if (other1 < other2) {
								p[rt[it] = rs[i1++]] = it; ++it;
								while(i1 < mid && p[(rs[i1]+t)%nm2] < other2) p[rt[it] = rs[i1++]] = it, ++it;
								i = it;
							} else {
								p[rt[it] = rs[i2++]] = it; ++it;
								while(i2 < j && p[(rs[i2]+t)%nm2] < other1) p[rt[it] = rs[i2++]] = it, ++it;
								i = it;
							}
						}
						while(i1 < mid) p[rt[it] = rs[i1++]] = it, ++it;
						while(i2 < j) p[rt[it] = rs[i2++]] = it, ++it;
						while(ini < it) rs[ini] = rt[ini], ++ini;
					}
				}
			}
		}
		if (nubi == 0) break;
		//forn(i,nm2) printf("%u(%u)  ", rs[i], p[rs[i]]); printf("\n");
		/*t*=2; printf ("---%d---\n",t);show(s,r); t/=2;*/
	}
	/* In case the solution is in the wrong array */
	if (rs != r) memcpy(r, rs, nm2 * sizeof(uint));

	// Antes de hacer PERCHA p, me acuerdo dónde quedó la string original
	if (prim) *prim = p[0];

	pz_free(mrs);
	pz_free(bc);
}
