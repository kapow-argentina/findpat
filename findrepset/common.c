#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include "common.h"

#include "macros.h"

#ifdef HAVE___BUILTIN_CLZ
uint bneed(uint x) { return x?32-__builtin_clz(x):0; }
#else
uint bneed(uint x) { uint res = 0; while (x) res++,x>>=1; return res; }
#endif

void endian_swap_16(ushort* x) { *x = ((*x >> 8) & 0x00ff) | ((*x << 8) & 0xff00); }

void endian_swap_32(uint* x) {
	*x = ((*x >> 8 ) & 0x00ff00ff) | ((*x << 8 ) & 0xff00ff00);
	*x = ((*x >> 16) & 0x0000ffff) | ((*x << 16) & 0xffff0000);
}

void endian_swap_64(uint64* x) {
	*x = ((*x >> 8LL ) & 0x00ff00ff00ff00ffLL) | ((*x << 8LL) & 0xff00ff00ff00ff00LL);
	*x = ((*x >> 16LL) & 0x0000ffff0000ffffLL) | ((*x << 16LL) & 0xffff0000ffff0000LL);
	*x = ((*x >> 32LL) & 0x00000000ffffffffLL) | ((*x << 32LL) & 0xffffffff00000000LL);
}

#define CHUNK 16384

/** Chunck reading **/
#define new_chunck \
	if (i == m) { \
		m = m?2*m:8; \
		tmp = pz_malloc(m*sizeof(uchar*)); \
		if (i) memcpy(tmp, mat, i*sizeof(uchar*)); \
		if (mat != NULL) pz_free(mat); \
		mat = tmp; \
	} \
	mat[i] = (uchar*)pz_malloc(CHUNK * sizeof(uchar)); i++;
#define drop_chunks while (i--) pz_free(mat[i]); if (mat != NULL) pz_free(mat);

void *_pz_malloc(size_t n, char *file, int line) {
	void *p;
	if (!(p = malloc(n))) {
		fprintf(stderr, "%s:%d MALLOC of %u bytes failed\n", file, line, (unsigned int)n);
		fflush(stderr);
	}
#if _DEBUG_LOG_MALLOC
	fprintf(stderr, "%s:%d log pz_malloc(%u bytes) => %p\n", file, line, (unsigned int)n, p);
	fflush(stderr);
#endif
	return p;
}

void _pz_free(void *ptr, char *file, int line) {
#if _DEBUG_LOG_MALLOC
	fprintf(stderr, "%s:%d log pz_free(%p)\n", file, line, ptr);
	fflush(stderr);
#endif
	free(ptr);
}

/**
 * Lee un archivo string a un buffer contiguo alocado por esta función y lo devuelve.
 */
uchar* loadStrFile(const char* filename, uint* n) {
	return loadStrFileExtraSpace(filename, n, 0);
}

uchar* loadStrFileExtraSpace(const char* filename, uint* n, uint esp) {
	uchar* res;
	fprintf(stderr, "Loading file %s ", filename);
	FILE* f = fopen(filename, "r");
	if (!f) {
		fprintf(stderr, "[%s]\n", strerror(errno));
//		perror("fopen");
		return NULL;
	}
	res = loadFileExtraSpace(f, n, esp);
	fclose(f);
	fprintf(stderr, "[OK]\n");
	return res;
}

/**
 * Writes a mem buffer to a new or existant file. Returns true if success.
 */
bool saveStrFile(const char* fn, const void* buf, uint n) {
	bool res;
	FILE* f = fopen(fn, "wb");
	if (!f) return 0;
	res =  n == fwrite(buf, 1, n, f);
	fclose(f);
	return res;
}


/**
 * Lee un archivo FILE* a un buffer contiguo alocado por esta función y lo devuelve.
 */
uchar* loadFile(FILE* f, uint* n) {
	return loadFileExtraSpace(f, n, 0);
}

/**
 * Lee un archivo FILE* a un buffer contiguo alocado por esta función y lo devuelve.
 */
uchar* loadFileExtraSpace(FILE* f, uint* n, uint esp) {
	uchar **mat = NULL, **tmp;
	uchar *res;
	uint r, i=0, m=0;
	do {
		new_chunck;
		r = fread(mat[i-1], 1, CHUNK, f);
	} while (r == CHUNK);
	// Ensamble chunks
	if (n != NULL) *n = r + CHUNK * (i-1);
	res = (uchar*)pz_malloc(r + CHUNK * (i-1) + esp);
	if (r) memcpy(res+(CHUNK*(i-1)), mat[i-1], r);
	pz_free(mat[--i]);
	while (i--) {
		memcpy(res+(CHUNK*i), mat[i], CHUNK);
		pz_free(mat[i]);
	}
	pz_free(mat);
	return res;
}

/**
 * Writes a mem buffer to a FILE*. Returns true if success.
 */
bool saveFile(FILE* f, const void* buf, uint n) {
	return n == fwrite(buf, 1, n, f);
}

/**
 * Returns TRUE if file "fn" exists
 */
bool fileexists(const char* fn) {
  struct stat buf;
  return stat(fn, &buf) != -1;
}

/**
 * Returns the size of "fn" or -1 if error.
 */
int filesize(const char* fn) {
  struct stat buf;
  if (stat(fn, &buf) == -1) return -1;
  return buf.st_size;
}


#ifdef GZIP
#include <zlib.h>

/**
 * Lee un archivo por nombre, comprimido con gzip, a un buffer contiguo alocado por esta función y lo devuelve.
 */
uchar* loadStrGzFile(const char* filename, uint* n) {
	uchar **mat = NULL, **tmp;
	uchar *res;
	uint r=CHUNK, i=0, m=0;
	fprintf(stderr, "Loading file %s ", filename);
	gzFile f = gzopen(filename, "rb");
	if (!f) {
//		perror("gzopen");
		fprintf(stderr, "[%s]\n", strerror(errno));
		return NULL;
	}
	do {
		new_chunck;
		r = gzread(f, mat[i-1], CHUNK);
	} while (r == CHUNK);
	// Ensamble chunks
	if (n != NULL) *n = r + CHUNK * (i-1);
	res = (uchar*)pz_malloc(r + CHUNK * (i-1));
	if (r) memcpy(res+(CHUNK*(i-1)), mat[i-1], r);
	pz_free(mat[--i]);
	while (i--) {
		memcpy(res+(CHUNK*i), mat[i], CHUNK);
		pz_free(mat[i]);
	}
	pz_free(mat);
	gzclose(f);
	fprintf(stderr, "[OK]\n");
	return res;
}

/**
 * Writes a mem buffer to a new or existant file and compress it with gzip. Returns true if success.
 */
bool saveStrGzFile(const char* filename, const void* buf, uint n) {
	uint res;
	gzFile f = gzopen(filename, "wb");
	if (!f) return 0;
	res = gzwrite(f, buf, n);
	gzclose(f);
	return res == n;
}

#endif

