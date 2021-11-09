#ifndef __KPW_H__
#define __KPW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tipos.h"
#include <stdio.h>

typedef struct {
	uint64 offset;
	uint64 size;
	uint type;
} __attribute__((packed)) kpw_index_ent;

typedef struct str_kpw_index {
	uint64 offset;
	uint zidx;
	struct str_kpw_index* next;
	kpw_index_ent idx[];
} kpw_index;

typedef struct {
	uchar magic[4]; /* "KPW",0 */
} __attribute__((packed)) kpw_header;

/* kpw index iterator */
typedef struct str_kpw_idx_it {
	kpw_index* idx;
	uint i, sn;
} kpw_idx_it;
#define KPW_IT(it) ((it).idx->idx+((it).i))
#define KPW_IT_END(it) ((it).idx == NULL)

#define KPW_ID_EMPTY 0

/* User defined */
#define KPW_MAX_SECTION_ID 7
#define KPW_ID_STATS	1
#define KPW_ID_BWT	2
#define KPW_ID_TRAC	3
#define KPW_ID_PATT	4
#define KPW_ID_LCP	5
#define KPW_ID_LCPS	6

extern const char* _kpw_str_id[KPW_MAX_SECTION_ID];
#define KPW_IDS		KPW_MAX_SECTION_ID
#define KPW_STRID(t)	((uint)(t) < KPW_IDS ? _kpw_str_id[(uint)(t)] : "<INVALID>")

#define KPW_FIX_ENDIANNESS	1

#if KPW_FIX_ENDIANNESS

#define KPW_fix_uint(X)	\
	((X) = ((X) >> 24) \
	| (((X) << 8) & 0x00ff0000) \
	| (((X) >> 8) & 0x0000ff00) \
	| ((X) << 24))

#define KPW_fix_uint64(X)	\
	((X) = ((X) >> 56) \
	| (((X) << 40) & 0x00ff000000000000) \
	| (((X) << 24) & 0x0000ff0000000000) \
	| (((X) << 8)  & 0x000000ff00000000) \
	| (((X) >> 8)  & 0x00000000ff000000) \
	| (((X) >> 24) & 0x0000000000ff0000) \
	| (((X) >> 40) & 0x000000000000ff00) \
	| ((X) << 56))

#endif /* KPW_FIX_ENDIANNESS */

typedef struct {
	FILE *f;
	kpw_index* idx;
	uint64 noffset;
	kpw_index* cur; uint curi; uint64 cursz;
#if KPW_FIX_ENDIANNESS
	bool swap_endian;
#endif
} kpw_file;

kpw_file* kpw_create(const char* filename);
kpw_file* kpw_open(const char* filename);
void kpw_close(kpw_file* this);
void kpw_list(kpw_file* this);
void kpw_show_sec(int i, kpw_index_ent* e);

/* kpw index iterator */
kpw_idx_it kpw_idx_begin(kpw_file* this);
kpw_idx_it kpw_idx_end(kpw_file* this);
void kpw_idx_next(kpw_idx_it* it);
void kpw_idx_add(kpw_idx_it* it, uint x);
void* kpw_idx_sec_load(kpw_file* this, kpw_idx_it *it, uint64* size);
kpw_idx_it kpw_sec_find(const kpw_file* this, uint type);

void kpw_sec_new(kpw_file* this, uint type);
void kpw_sec_close(kpw_file* this);
int kpw_sec_write(kpw_file* this, const void* buf, uint64 size);
void kpw_sec_write_upd(kpw_file* this);
void kpw_sec_delete(kpw_file* this, const kpw_idx_it it);

FILE* kpw_sec_fileh(kpw_file* this, uint type, uint64* size);
FILE* kpw_idx_fileh(kpw_file* this, const kpw_idx_it it, uint64* size);
int kpw_sec_read(kpw_file* this, void* buf, uint64 size);
void* kpw_sec_load(kpw_file* this, uint type, uint64* size);
bool kpw_sec_exists(kpw_file* this, uint type, uint64* size);
uint64 kpw_sec_load_into(kpw_file* this, uint type, void* buf, uint64 size);
uint64 kpw_sec_save_from(kpw_file* this, uint type, void* buf, uint64 size);

#ifdef __cplusplus
}
#endif

#endif
