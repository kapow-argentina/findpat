#ifndef __MACROS_C__
#define __MACROS_C__
#include <stdio.h>

#define forn(i, n) for(i = 0; i < (n); ++i)
#define dforn(i, n) for(i = (n); i-- != 0;)
#define forsn(i, s, n) for(i = (s); i < (n); ++i)

#define ppz_error(F, ...) fprintf(stderr, "%s:%d: "F"\n", __FILE__, __LINE__, __VA_ARGS__);

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif


#ifdef _DEBUG
#define DBGu(X) fprintf(stderr, "%s:%d: %s = %u\n", __FILE__, __LINE__, #X, (uint)(X));
#define DBGd(X) fprintf(stderr, "%s:%d: %s = %d\n", __FILE__, __LINE__, #X, (int)(X));
#define DBGx(X) fprintf(stderr, "%s:%d: %s = %.8x\n", __FILE__, __LINE__, #X, (uint)(X));
#define DBGlx(X) fprintf(stderr, "%s:%d: %s = %.16lx\n", __FILE__, __LINE__, #X, (unsigned long)(X));
#define DBGs(X) fprintf(stderr, "%s:%d: %s = %s\n", __FILE__, __LINE__, #X, (char*)(X));

#define DBG_n(md, X, N) { fprintf(stderr, "%s:%d: %s = ", __FILE__, __LINE__, #X); int _; forn(_,(int)(N)) fprintf(stderr, md, (X)[_]); fprintf(stderr, "\n"); }

#else
#define DBGu(X)
#define DBGd(X)
#define DBGx(X)
#define DBGlx(X)
#define DBGs(X)
#define DBG_n(md, X, N)
#endif // _DEBUG

#define DBGcn(X, N) DBG_n("%c", X, N)
#define DBGun(X, N) DBG_n("%u ", X, N)
#define DBGlxn(X, N) DBG_n("%llx ", X, N)

#ifdef _DEBUG
void *_pz_malloc(size_t n, char *file, int line);
void _pz_free(void *ptr, char *file, int line);
#define pz_malloc(n)	_pz_malloc(n, __FILE__, __LINE__)
#define pz_free(p)	_pz_free(p, __FILE__, __LINE__)
#else
#define pz_malloc malloc
#define pz_free free
#endif

#endif // __MACROS_C__
