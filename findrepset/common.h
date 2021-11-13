#ifndef __COMMON_H__
#define __COMMON_H__

#include "tipos.h"
#include <stdio.h>

/** Integer logarithm **/
uint bneed(uint x) __attribute__((const,always_inline));

void endian_swap_16(ushort* x) __attribute__((always_inline));
void endian_swap_32(uint* x) __attribute__((always_inline));
void endian_swap_64(uint64* x) __attribute__((always_inline));

void *_pz_malloc(size_t n, char *file, int line);
void _pz_free(void *ptr, char *file, int line);

uchar* loadStrFile(const char*, uint* n);
uchar* loadStrFileExtraSpace(const char*, uint* n, uint esp);
bool saveStrFile(const char* fn, const void* buf, uint n);
uchar* loadFile(FILE* f, uint* n);
uchar* loadFileExtraSpace(FILE*, uint* n, uint esp);
bool saveFile(FILE* f, const void* buf, uint n);

bool fileexists(const char* fn);
int filesize(const char* fn);

#ifdef GZIP
uchar* loadStrGzFile(const char*, uint* n);
bool saveStrGzFile(const char* filename, const void* buf, uint n);
#endif

/** Command line "functions" **/

#define cmdline_foreach(i) for(i = 0; i < argc; ++i)
#define cmdline_is_opt(i) (argv[i][0] == '-' || argv[i][0] == '+')
#define cmdline_opt_1(i, opt) if (!strcmp(opt, argv[i]))
#define cmdline_opt_2(i, opt) if ((i+1 < argc) && !strcmp(opt, argv[i]) && ++i)
#define cmdline_var(i, opt, var) if (cmdline_is_opt(i) && !strcmp(opt, argv[i]+1)) { var = argv[i][0] == '-'; }

#endif
