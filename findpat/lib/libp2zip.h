#ifndef __LIBP2ZIP_H__
#define __LIBP2ZIP_H__

#include "kpw.h"
#include "tipos.h"
#include "bitstream.h"

#define PZ_ST_CREATED 0
#define PZ_ST_LCP     1
#define PZ_ST_LOADED  2
#define PZ_ST_FILTER  3
#define PZ_ST_PREBWT  4
#define PZ_ST_BWT     5
#define PZ_ST_MTF     6
#define PZ_ST_LZW     7

#define PZ_ST_ERROR   0xFF

#define PZ_GZ_LEVEL 9
#define KPW_FLAGS_GZ 0x01

#define MAX_FILENAME_LEN 128

typedef struct {
	/* XXX:
	 *   If this struct changes, the following function:
	 *     - kpwapi.c:endian_swap_stats(ppz_stats* data)
	 *     - kpw_check.c:kpw_check_fix_stats
	 *
	 *   MUST be changed to correctly read the values stored
	 *   in kpw files written in big-endian architectures.
	 */

	uint rev;                          /* Source revision number of the code that generates this stats */
/* Statistical members */
	uint long_source_a, long_source_b; /* Long of input file */
	uint long_input_a, long_input_b;   /* Long of preprocesed input file (without Ns, before BWT) */
	uint n, nlzw;
	uint zero_mtf;                     /* How many 0s in the output of BWT+MTF */
	uint nzz_est;                      /* How many non-0 followed by 0 are in the output of BWT+MTF */
	uint long_output;                  /* Long (in number of codes) of BWT+MTF+LZW */
	uint est_size;                     /* Estimated size of BWT+MTF+LZW+ARIT_ENC (in bytes) */
	uint mtf_entropy;                  /* Estimated size of BWT+MTF+ARIT_ENC (in bytes) */
	uint huf_len, huf_tbl_bit;         /* Long of Huffman output (in bytes) and, long of huffman table (in bits) */
	uint lzw_codes;
	uint64 lpat_size, lpat3_size;        /* Estimated size for longpat, for ch=8, and ch=3, (in bits) */
	char filename_a[MAX_FILENAME_LEN];
	char filename_b[MAX_FILENAME_LEN];
	char hostname[MAX_FILENAME_LEN];

/* Stats for biop2zip */
	uint cnt_A, cnt_C, cnt_G, cnt_T, cnt_N; /* Count how many ACGTN are in the real input */
	uint N_blocks; /* How many blocks of more than 5 N there are. */

/* Time stats */
	uint time_bwt, time_mtf, time_lzw,
		time_mrs, time_huf, time_longp, time_tot; /* in ms. wrap in 49 days X-) */
/* New params */
	uint64 lpat3dl0_size;
	double kinv_lcp;
	uint time_mmrs;
} __attribute__((packed)) ppz_stats;

typedef struct {
	uint st;
	uint *p, *r; /* suffix array */
	uint *h; /* LCP */
	uchar *s, *bwto;
	ushort *lzw_cds; /* lzw_cds= códigos de lzw */
	uint no; /* n=source-len, no=output-len */
	uint prim;  /* Row of the BWT output with the original array */

	uint gz_input, fa_input, fa_strip_n, use_terminator, use_separator;
	uint trac_positions; /* Tells if we need to trac real positions for FASTA files */
	uint *trac_buf, trac_size, trac_middle;

	kpw_file* kpwf;
	bool update_data;
	ppz_stats data;

	bool fix_endianness;

} ppz_status;

void ppz_create(ppz_status* this);
void ppz_destroy(ppz_status* this);

void ppz_compress_gen(ppz_status* this, void (*bwtf)(uchar*, uint*, uint*, uchar*, uint, uint*));
void ppz_compress(ppz_status* this);
void ppz_ocompress(ppz_status* this);
void ppz_decompress(ppz_status* this);

/* pseudo-private members */
int ppz_load_file(ppz_status* this, const char* fn);
int ppz_load_file2(ppz_status* this, const char* fn1, const char* fn2);
int ppz_load_bwt(ppz_status* this, const char* fn);
int ppz_save_bwt(ppz_status* this, const char* fn);
void ppz_bwt_gen(ppz_status* this, void (*bwtf)(uchar*, uint*, uint*, uchar*, uint, uint*));
void ppz_bwt(ppz_status* this);
void ppz_obwt(ppz_status* this);
void ppz_lcp(ppz_status* this, uint save);
void ppz_klcp(ppz_status* this, uint save);
void ppz_longpat(ppz_status* this);
void ppz_mrs(ppz_status* this, uint save, uint ml);
void ppz_mrs_nolcp(ppz_status* this, uint save, uint ml);
void ppz_mmrs(ppz_status* this, uint save, uint ml);
void ppz_mtf(ppz_status* this);
void ppz_mtf_ests(ppz_status* this);
void ppz_lzw(ppz_status* this);
void ppz_write_out(ppz_status* this, const char* fn);
void ppz_write_stats(ppz_status* this, const char* fn);
void ppz_est_size(ppz_status* this);
void ppz_huffman_len(ppz_status* this);
uint ppz_trac_convert(ppz_status* this, uint x);
void ppz_trac_load(ppz_status* this, const char* fn);

void ppz_kpw_open(ppz_status* this, const char* fn);
bool ppz_kpw_load_stats(ppz_status* this);
void ppz_kpw_save_stats(ppz_status* this);
bool ppz_kpw_load_trac(ppz_status* this);
bool ppz_kpw_save_trac(ppz_status* this);
int ppz_kpw_load_bwt(ppz_status* this);
bool ppz_kpw_save_bwt(ppz_status* this);

/* Builds string: [dirname(outn)]/[pfx]-[basename(in1)]-[basename(in2)][ext] */
char* std_name(const char* outn, const char* pfx, const char* in1, const char* in2, const char* ext);

/*** Functions to deal with kpw files by name ***/

/* If the corresponding .kpw file doesn't exist, these
 * functions create it first. */

#define PPZ_MAX_KPW_FILENAME 	1024
#define PPZ_LEFT		1
#define PPZ_RIGHT		2
#define PPZ_NEW			3

/* Returns PPZ_LEFT when it finds path/file1-file2.kpw
 * and PPZ_RIGHT when it finds path/file2-file1.kpw
 * Creates path/file1-file2.kpw and return PPZ_NEW when neither of them exist. 
 * kpwfn is the output; it should be a buffer of at least PPZ_MAX_KPW_FILENAME bytes */
int ppz_kpwfile_find(ppz_status *pz, char *kpwfn, char *path, char *file1, char *file2);

/* Loads the DATA, BWT and TRAC sections. Returns PPZ_LEFT if it is
 * the BWT of file1 + file2 and PPZ_RIGHT if it is the BWT of file2 + file1.
 * After this, pz.p, pz.r and pz.s are the expected arrays. */
int ppz_kpwfile_load(ppz_status *pz, char *path, char *file1, char *file2);

#endif
