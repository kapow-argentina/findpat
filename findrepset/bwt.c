#include "bwt.h"
#include "sorters.h"
#include "macros.h"

#include <stdlib.h>
#include <string.h>

#define QSORTUB (16*1024)  /* QSORT / BSORT  limit. */
#define BSORTBITS 16
#define BSORTSIZE (64*1024) /* 2^BSORTBITS */

/* Parallel sort */
#ifdef BWT_PSORT
#include "psort.h"
psort bwt_ps;
#endif

static __thread uint n;   /* largo de la entrada  */
static __thread uint* p;  /* numero de posicion actual de cada rotacion */
static __thread uint* r;  /* permutación resultado con el orden */
static __thread uint t;   /* 2^(numero de pasos) */
static __thread uint* bc; /* memoria para el bucket sort */
static __thread uint64* qm;/* memoria para el qsort por copia */


/*** DEBUG functions ***/
void show(uchar* s, uint* r);

/*** Prototypes: sorters ***/
static void internal_bsort(uint* b, uint* e);
static inline void internal_sort_M32(uint* b, uint* e);
void internal_qsort3(uint* b, uint* e);

static inline void fix_index(uint *b, uint *e);


/*** Sorters ***/

#define SWAP(a,b) {*(a)^=*(b); *(b)^=*(a); *(a)^=*(b);}

/*** Macro de bucket-sort, para lo y hi ***/
#define internal_bsort_partial(name, yyy, xxx) \
void name(uint* b, uint* e) { \
	uint *pp, *np; \
	uint i,y; \
	if (e-b < 2) return; \
	forn(i,BSORTSIZE) bc[i]=0; \
	forsn(pp,b,e) bc[(yyy p[((*pp)+t)%n]) xxx]++; \
	y = bc[0] != 0; \
	forsn(i,1,BSORTSIZE) { y+=bc[i]!=0; bc[i]+=bc[i-1]; } \
	if (y <= 1) return; /* hay un sólo bucket, no way */ \
	for(pp=b;pp!=e;) { \
		y = (yyy p[((*pp)+t)%n]) xxx; \
		np = b+(bc[y]-1); \
		if (np > pp) { \
			SWAP(pp,np); \
			--bc[y]; \
		} else { \
			++pp; \
		} \
	} \
}

int check_invariant(void) {
	uint *c, bq=0;
	uint *b=r, *e=r+n;
	for(c = b; c < e; ++c) {
		if (p[*c] > c-b) {
			fprintf(stderr, "** Bucket %u starts at position %u (%u positions before)\n", p[*c], (uint)(c-b), p[*c]-(uint)(c-b));
			exit(1);
			return 0;
		}
		if (p[*c] != bq) {
			if (p[*c] != c-b) {
				fprintf(stderr, "** New bucket %u starts, but at position %u (last bucket: %u)\n", p[*c], (uint)(c-b), bq);
				exit(1);
				return 0;
			}
			bq = p[*c];
		}
	}
	return 1;
}

/*** Instanciaciones del bsort, para lo y hi ***/
internal_bsort_partial(internal_bsort_hi, , >> BSORTBITS)
internal_bsort_partial(internal_bsort_lo, ,& (BSORTSIZE-1))

/*** BSORT de indices sobre la estructura de 2 arrays ***/
void internal_bsort(uint* b, uint* e) {
	uint* pp, *ee = e;
	uint pb;
	internal_bsort_hi(b,e);
	for(pp = e-1; b < e; e = pp+1) {
		pb = p[((*pp)+t)%n] >> BSORTBITS;
		while(b <= pp && ((p[((*pp)+t)%n] >> BSORTBITS) == pb)) --pp;
		if (e-pp < QSORTUB) {
//			internal_sort_M32(pp+1,e);
			internal_qsort3(pp+1,e);
		} else {
			internal_bsort_lo(pp+1,e);
		}
	}
	fix_index(b, ee);
//	check_invariant();
}

/*** QSORT de indices sobre la estructura de 2 arrays ***/
#define _VAL(X) (p[(*(X)+t)%n])
_def_qsort3(internal_qsort3, uint, uint, _VAL, <)
#undef _VAL

/*** QSORT de indices que copia a otra estructura en un vector de elementos de 64bits. ***/
#define val_qsortM32(X) ((uint)*(X))
_def_qsort3(internal_qsortM32, uint64, uint, val_qsortM32, <)

static inline void internal_sort_M32(uint* b, uint* e) {
	uint *c, np, vl, nvl, d;
	uint mn = e-b;
	uint64 *mu = qm;
	for(c=b; c!=e; ++c, ++mu) {
		*mu = (uint64)(p[((*c)+t)%n]) | ((uint64)*c << 32);
	}
#ifdef BWT_PSORT
	psort_job_new(&bwt_ps, &(psort_job){qm, qm+mn});
#else
	internal_qsortM32(qm,qm+mn);
#endif
	/* simpler fix_index ad-hoc */
	mu = qm;
	np = b-r;
	vl = val_qsortM32(mu);
	d = 0;
	for(c=b; c!=e; ++c, ++mu) {
		if (vl != (nvl = (val_qsortM32(mu)))) {
			vl = nvl;
			d = (c-b);
		}
		p[*c = (*(mu) >> 32)] = np+d; /* Hi qsort value */
	}
}

/*** Actualiza la estructura luego de ordenar un bucket ***/
static inline void fix_index(uint *b, uint *e) {
	uint pkm1, pk, np, i, d, m;
	pkm1 = p[(*b+t)%n];
	m = e-b; d = 0;
	np = b-r; /* Id del bucket = posicion dentro del resultado */
	forn(i, m) {
		if (((pk = p[(*b+t)%n]) != pkm1) && !(np <= pkm1 && pk < np+m)) {
			pkm1 = pk;
			d = i;
		}
		p[*(b++)] = np+d;
	}
}

/*** generic sort function ***/
static inline int internal_sort(uint* b, uint* e) {
	if (e-b <= 1) {
		return 1;
	} else if (e-b < QSORTUB) {
		internal_sort_M32(b,e);
		return 0;
	} else {
		internal_bsort(b,e);
		return 0;
	}
}

/**
 * Función de BWT para usar 8*n RAM
 */
void bwt(uchar *bwt, uint* pp, uint* rr, uchar* src, uint nn, uint* prim) {
#define CONCAT(_F,_S) ((((ushort)_F) << 8) | ((ushort)_S))
	uint i,j,lnb=0,nb = 1;
	uchar *s = src?src:(uchar*)pp;
	uint c[256];
	bc = (uint*)pz_malloc(BSORTSIZE * sizeof(uint));
	n = nn;
	p = pp; r = rr;
	t = 1;
	memset(bc, 0, sizeof(uint)*BSORTSIZE);
	memset(c, 0, sizeof(c));
	forn(i,n-1) ++bc[CONCAT(s[i],s[i+1])], ++c[s[i]]; //calcular frecuencias (de 16 y de 8 bits en la misma pasada, para romper menos la cache)
	++bc[CONCAT(s[n-1],s[0])]; ++c[s[n-1]];

	/* Calcula la frecuencias acumuladas incluyendo hasta el índice dado */
	forsn(i, 1, BSORTSIZE) { bc[i]+=bc[i-1]; }

	/* Inicializa el vector de indices */
	forn(i, n-1) r[--bc[CONCAT(s[i],s[i+1])]] = i;
	r[--bc[CONCAT(s[n-1],s[0])]] = n-1;
	/* Esto deja las frecuencias acumuladas sin incluir el índice dado */

	//inicializar numero de posicion segun primer caracter-doble
	p[n-1] = bc[CONCAT(s[n-1],s[0])];
	dforn(i, n-1) p[i]=bc[CONCAT(s[i],s[i+1])];

	qm = (uint64*)pz_malloc(QSORTUB*sizeof(uint64)); /* Memoria para el qsort por copia */

#ifdef BWT_PSORT
	psort_init(&bwt_ps, 4, psort_thread);
#endif

	for(t = 2; t < n; t*=2) {
		lnb = nb;
		nb = 0;
		for(i = 0, j = 1; i < n; i = j++) {
			/*calcular siguiente bucket*/
			while(j < n && p[r[j]] == p[r[i]]) ++j;
			internal_sort(r+i, r+j);
			nb++;
		}
		if (lnb == nb) break;
		/*t*=2; printf ("---%d---\n",t);show(s,r); t/=2;*/
	}
#ifdef BWT_PSORT
	psort_destroy(&bwt_ps);
#endif

	// Antes de hacer PERCHA p, me acuerdo dónde quedó la string original
	if (prim) *prim = p[0];

	bwt_src_bc(bwt, p, r, src, n, c);
	pz_free(bc);
	pz_free(qm);
}

void obwt(uchar *bwt, uint* pp, uint* rr, uchar* src, uint nn, uint* prim) {
#define CONCAT(_F,_S) ((((ushort)_F) << 8) | ((ushort)_S))
	uint i,j,k,lnb=0,nb = 1;
	uchar *s = src?src:(uchar*)pp;
	uint c[256];
	bc = (uint*)pz_malloc(BSORTSIZE * sizeof(uint));
	int *l;
	
	n = nn;
	p = pp; r = rr;
	t = 1;
	/* array to store the lengths of groups to skip */
	l = (int*)pz_malloc(n * sizeof(int));
	memset(bc, 0, sizeof(uint)*BSORTSIZE);
	memset(c, 0, sizeof(c));
	memset(l, 0, n * sizeof(int));
	forn(i,n-1) ++bc[CONCAT(s[i],s[i+1])], ++c[s[i]]; //calcular frecuencias (de 16 y de 8 bits en la misma pasada, para romper menos la cache)
	++bc[CONCAT(s[n-1],s[0])]; ++c[s[n-1]];

	/* Calcula la frecuencias acumuladas incluyendo hasta el índice dado */
	forsn(i, 1, BSORTSIZE) { bc[i]+=bc[i-1]; }

	/* Inicializa el vector de indices */
	forn(i, n-1) r[--bc[CONCAT(s[i],s[i+1])]] = i;
	r[--bc[CONCAT(s[n-1],s[0])]] = n-1;
	/* Esto deja las frecuencias acumuladas sin incluir el índice dado */

	//inicializar numero de posicion segun primer caracter-doble
	p[n-1] = bc[CONCAT(s[n-1],s[0])];
	dforn(i, n-1) p[i]=bc[CONCAT(s[i],s[i+1])];

	qm = (uint64*)pz_malloc(QSORTUB*sizeof(uint64)); /* Memoria para el qsort por copia */

#ifdef BWT_PSORT
	psort_init(&bwt_ps, 4, psort_thread);
#endif

	for(t = 2; t < n; t*=2) {
		lnb = nb;
		nb = 0;
		for(i = 0, j = 1; i < n; i = j++) {
			/* position of the first sorted group to merge */
			k = i;
			/* if the length is negated, it is already sorted */ 
			while(i < n && l[i] < 0) 
				i -= l[i];
			l[k] = k - i;
			j = i + 1;
			/*calcular siguiente bucket*/
			while(j < n && p[r[j]] == p[r[i]]) ++j;
			if (internal_sort(r+i, r+j) && i < n) l[i] = -1;
			nb++;
		}
		if (lnb == nb) break;
		/*t*=2; printf ("---%d---\n",t);show(s,r); t/=2;*/
	}
#ifdef BWT_PSORT
	psort_destroy(&bwt_ps);
#endif

	// Antes de hacer PERCHA p, me acuerdo dónde quedó la string original
	if (prim) *prim = p[0];

	bwt_src_bc(bwt, p, r, src, n, c);
	pz_free(l);
	pz_free(bc);
	pz_free(qm);
}


/** Inversa de BWT **/
void ibwt(uchar *src, uchar *dst, uint n, uint prim) {
	uint i,j,sum;
	uint *ind = (uint*)pz_malloc(n * sizeof(uint));
	bc = (uint*)pz_malloc(256 * sizeof(uint));
	memset(bc, 0, 256 * sizeof(uint));
	forn(i, n) ind[i] = bc[src[i]]++;
	sum = 0;
	forn(i, 256){
		register uint __t = bc[i];
		bc[i] = sum;
		sum += __t;
	}
	j = prim;
	dforn(i, n) {
		dst[i] = src[j];
		j = bc[src[j]] + ind[j];
	}
	pz_free(ind);
	pz_free(bc);
}

void bwt_src_bc(uchar *bwt, uint *p, uint *r, uchar *src, uint n, uint* bc) {
	uint i, j;
	/* Regenera la entrada en src */
	if (!src) src = ((uchar*)(p+n)) - n;
	j = 0;
	forn(i, 256) while (bc[i]--) src[r[j++]] = i;
	/* Reordena la entrada según r al principio de p */
	if (!bwt) bwt = (uchar*)p;
	bwt += n;
	dforn(i, n) *(--bwt) = src[(r[i]+n-1)%n];
}

void bwt_rsrc_pbc(uchar *bwt, uint *p, uint *r, uchar* src, uint n, uint* bc) {
	uint i;
	forn(i, n) r[p[i]] = i;
	bwt_src_bc(bwt, p, r, src, n, bc);
}

void bwt_build_bc(uchar* src, uint n, uint* bc) {
	uint i;
	memset(bc, 0, 256 * sizeof(uint));
	forn(i, n) bc[src[i]]++;
}

void bwt_spr(uchar *bwt, uint *p, uint *r, uchar *src, uint n, uint prim) {
	uint i,j,sum;
	if (!bwt) bwt = (uchar*)p;
	bc = (uint*)pz_malloc(256 * sizeof(uint));
	memset(bc, 0, 256 * sizeof(uint));
	forn(i, n) bc[bwt[i]]++;
	sum = 0;
	forn(i, 256){
		register uint t = bc[i]; bc[i] = sum; sum += t;
	}
	forn(i, n) { r[i] = bc[bwt[i]]++; }
	// Dejo de usar src[]
	// Calcula p[]
	j = r[prim];
	dforn(i, n) {
		p[i] = j;
		j = r[j];
	}
	// Calcula r[] dado p[]
	forn(i, n) r[p[i]] = i;
	// Calcula la entrada original dado p, r y la cantidad de cada caracter.
	dforn(i, 256) if (i) bc[i] -= bc[i-1];
	bwt_src_bc(bwt, p, r, src, n, bc);
	pz_free(bc);
}

/*** DEBUG ***/
void show(uchar* s, uint* r) {
	int i,j;
	forn(i,n) {
		printf("%d (%d,%d)", r[i], p[r[i]], p[(r[i]+t)%n]);
		if (i) forn(j,t) {
			char a=s[(r[i-1]+j)%n];
			char b=s[(r[i]+j)%n];
			if (a>b) printf("-------------- V -----------\n");
			if (a!=b) break;
		}
		forn(j,n) printf("%c", s[(r[i]+j)%n]); printf("\n");
	}
	printf("\n");
}
