#ifndef __KPWAPI_H__
#define __KPWAPI_H__

#include "libp2zip.h"

void endian_swap_stats(ppz_stats* data);
bool kpw_load_stats(kpw_file* kpwf, ppz_stats* data, uint size);
void kpw_save_stats(kpw_file* kpwf, ppz_stats* data);


/* KPW TRAC Format:
 * < uint flags >
 * < uint trac_size >
 * < uint trac_middle >
 * < data >
 *
 * if flags & KPW_FLAGS_GZ is on, data is compresed with gzip.
 */
typedef struct {
	uint flags;
	uint trac_size;
	uint trac_middle;
	uchar data[];
} __attribute__((packed)) kpw_trac_head;

uint* kpw_load_trac(kpw_file* kpwf, uint* t_size, uint* t_middle);
bool kpw_save_trac(kpw_file* kpwf, uint* trac_buf, uint t_size, uint t_middle);

/* KPW BWT Format:
 * < uint flags >
 * < uint64 n >
 * < uint64 prim >
 * < data >
 *
 * if flags & KPW_FLAGS_GZ is on, data is compresed with gzip.
 */
typedef struct {
	uint flags;
	uint64 n;
	uint64 prim;
	uchar data[];
} __attribute__((packed)) kpw_bwt_head;

uchar* kpw_load_bwt(kpw_file* kpwf, uint* len, uint* prim);
bool kpw_save_bwt(kpw_file* kpwf, uchar* bwto, uint len, uint prim);

/* KPW LCP Format:
 * < uint flags >
 * < uint64 n >
 * < data >
 *
 * if flags & KPW_FLAGS_GZ is on, data is compresed with gzip.
 */
typedef struct {
	uint flags;
	uint64 n;
	uchar data[];
} __attribute__((packed)) kpw_lcp_head;


uint *kpw_load_lcp(kpw_file* kpwf, uint64* len);
bool kpw_save_lcp(kpw_file* kpwf, uint* h, uint n);

uchar *kpw_load_lcpset(kpw_file* kpwf, uint64* len);
bool kpw_save_lcpset(kpw_file* kpwf, uchar* hset, uint n);

/* KPW - iterator for the PATT section */

/* Usage:

	ppz_kpw_patt_it patt_it;
	ppz_kpw_patt_it_create(pz, &patt_it);
	while ((cant_patterns = ppz_kpw_patt_next_subsection(&patt_it))) {
		current_pattern_length = patt_it.current_length;
		forn (i, cant_patterns) {
			cant_ocurrences = ppz_kpw_patt_next_pattern(&patt_it);
			current_pattern_index = patt_it.pattern_index;
			...
		}
	}
	ppz_kpw_patt_it_destroy(&patt_it);
 
 */
typedef struct str_ppz_kpw_patt_it {
	kpw_file* kpwf;
	FILE *inf;

	uint data_encr;
	uint64 tot_bytes;

	/* The PATT section has one subsection for each pattern length. */

	/* Data for the current subsection */
	ibitstream ibs;
	uint current_length;

	/* Each subsection has many patterns.
	 * Each pattern can occur more than once. */
	uint pattern_index; /* Index of r[] in in data_encr fixed bits (same bits as n) */
} ppz_kpw_patt_it;

/* Requires the data section to be loaded. */
void ppz_kpw_patt_it_create(ppz_status *this, ppz_kpw_patt_it *it);
/* Returns the number of distinct patterns in this subsection.
 * Returns 0 on end-of-section. */
uint ppz_kpw_patt_next_subsection(ppz_kpw_patt_it *it);
/* Returns the number of occurrences of this pattern. */
uint ppz_kpw_patt_next_pattern(ppz_kpw_patt_it *it);
void ppz_kpw_patt_it_destroy(ppz_kpw_patt_it *it);

typedef void (*ppz_kpw_patt_callback)(uint len, uint occs, uint idx);
void ppz_kpw_iterate(ppz_status *this, ppz_kpw_patt_callback callback);

#define KPW_PATT_INVALID 0xffffffff
bool kpw_sec_patt_subsec_open(kpw_file* kpwf);
bool kpw_sec_patt_subsec_close(kpw_file* kpwf, uint subsec_off, uint cur_length, uint cur_cant);

#endif
