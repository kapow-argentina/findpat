#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#include <unistd.h>
#include "common.h"
#include "kpw.h"

#include "macros.h"
#define _max(a, b) ((a)<(b)?(b):(a))
#define _min(a, b) ((a)>(b)?(b):(a))

const char * _kpw_str_id[] = { "<EMPTY>", "STATS", "BWT", "TRAC", "PATT", "LCP", "LCPS" };

#define kpw_min_idx 8
const kpw_header kpw_def = { { 'K', 'P', 'W', 0 } };

/* KPW FILE Format:
 *
 * < kpw_header >
 * < index_chunck >
 * < chunks >[]
 */

/* index_chunk format:
 *
 * < num_index_entries >
 * < next_offset >
 * < index_entrie >[num_index_entries]

 * index_entrie format: struct kpw_index_ent
 */

kpw_index* kpw_idx_alloc(uint sz) {
	kpw_index* res = pz_malloc(sizeof(kpw_index) + sizeof(kpw_index_ent) * sz);
	res->zidx = sz;
	memset(res->idx, 0, sizeof(kpw_index_ent) * sz);
	res->offset = 0;
	res->next = NULL;
	return res;
}

kpw_index* kpw_idx_load(kpw_file* kpwf, FILE* f) {
	uint zidx;
	uint64 off;
	kpw_index* res;
	if (!fread(&zidx, sizeof(zidx), 1, f)) return NULL;
	if (!fread(&off, sizeof(off), 1, f)) return NULL;

#if KPW_FIX_ENDIANNESS
	/* XXX: kludge. Swap endian when the number of sections is "too high" */
	kpwf->swap_endian = !!(zidx & 0xffff0000);
	if (kpwf->swap_endian) {
		KPW_fix_uint64(off);
		KPW_fix_uint(zidx);
	}
#endif
	res = kpw_idx_alloc(zidx);
	res->offset = off;
	if (!fread(res->idx, sizeof(kpw_index_ent), zidx, f)) {
		pz_free(res);
		return NULL;
	}

#if KPW_FIX_ENDIANNESS
	if (kpwf->swap_endian) {
		for (int i = 0; i < zidx; ++i) {
			KPW_fix_uint64(res->idx[i].offset);
			KPW_fix_uint64(res->idx[i].size);
			KPW_fix_uint(res->idx[i].type);
		}
	}
#endif
	return res;
}

int kpw_idx_write(const kpw_index* idx, FILE* f) {
	uint64 next_offset = idx->next?idx->next->offset:0;
	fseek(f, idx->offset, SEEK_SET);
	int wt = 0;
	wt += fwrite(&(idx->zidx), sizeof(idx->zidx), 1, f);
	wt += fwrite(&next_offset, sizeof(next_offset), 1, f);
	wt += fwrite(idx->idx, sizeof(kpw_index_ent), idx->zidx, f);
	return wt;
}

uint64 kpw_idx_size(const kpw_index* idx) {
	return sizeof(idx->zidx) + sizeof(idx->offset) + sizeof(kpw_index_ent) * idx->zidx;
}

kpw_file* kpw_create(const char* filename) {
	kpw_file* res;
	FILE* f = fopen(filename, "wb+");
	if (!f) return NULL;
	res = (kpw_file*)pz_malloc(sizeof(kpw_file));
	res->f = f;
	res->idx = kpw_idx_alloc(kpw_min_idx);
	res->cur = NULL;
	res->idx->offset = sizeof(kpw_header);

#if KPW_FIX_ENDIANNESS
	res->swap_endian = FALSE;
#endif

	if (!fwrite(&kpw_def, sizeof(kpw_header), 1, f)) {
		perror("Writing to new kpw file");
		pz_free(res->idx);
		pz_free(res);
		fclose(f);
		return NULL;
	}
	kpw_idx_write(res->idx, f);

	res->noffset = sizeof(kpw_header) + kpw_idx_size(res->idx);
//	ftruncate(f, res->noffset);
	return res;
}

kpw_file* kpw_open(const char* filename) {
	kpw_file* res;
	uint i;
	FILE* f = fopen(filename, "rb+");
	if (!f) return kpw_create(filename);

	kpw_header hd;
	if (!fread(&hd, sizeof(hd), 1, f)) {
		perror("ERROR reading kpw file header");
		fclose(f);
		return NULL;
	}
	if (memcmp(kpw_def.magic, hd.magic, 4)) { fclose(f); return NULL; }

	res = (kpw_file*)pz_malloc(sizeof(kpw_file));
	res->f = f;
	res->idx = kpw_idx_load(res, f);
	res->cur = NULL;

	kpw_index* p = res->idx;
	uint64 tmp, off = p->offset;
	p->offset = sizeof(kpw_header);
	while (off) {
		fseek(f, off, SEEK_SET);
		p->next = kpw_idx_load(res, f); p = p->next;
		tmp = p->offset; p->offset = off; off = tmp;
	}

	res->noffset = sizeof(kpw_header) + kpw_idx_size(res->idx);
	for (p = res->idx; p != NULL; p = p->next) {
		forn(i, p->zidx) res->noffset = _max(res->noffset, p->idx[i].offset + p->idx[i].size);
	}
	return res;
}

void kpw_close(kpw_file* this) {
	kpw_index *p, *q;
	if (this->cur) kpw_sec_close(this);
//	kpw_list(this);
	for (p = this->idx; p != NULL; p = q) { q = p->next; pz_free(p); }
	if (this->f) fclose(this->f);
	pz_free(this);
}

void kpw_list(kpw_file* this) {
	kpw_index *p;
	uint i = 0, j;
	for (p = this->idx, j=0; p != NULL; p = p->next) {
		forn(i, p->zidx) { fprintf(stderr, "%2u: id=%2u, offset=%8llx, size=%8llu\n", j, p->idx[i].type, p->idx[i].offset, p->idx[i].size); j++; }
	}
}

/* kpw index iterator */
kpw_idx_it kpw_idx_begin(kpw_file* this) {
	return (kpw_idx_it){this->idx, 0, 0};
}
kpw_idx_it kpw_idx_end(kpw_file* this) {
	return (kpw_idx_it){this->idx, 0, 0};
}
void kpw_idx_next(kpw_idx_it* it) {
	it->i++;
	it->sn++;
	if (it->i >= it->idx->zidx) {
		it->i = 0;
		it->idx = it->idx->next;
	}
}
void kpw_idx_add(kpw_idx_it* it, uint x) {
	while (it->idx && x >= it->idx->zidx - it->i) {
		x -= it->idx->zidx - it->i;
		it->sn += it->idx->zidx - it->i;
		it->idx = it->idx->next;
		it->i = 0;
	}
	if (!it->idx) return;
	it->i += x;
}

void* kpw_idx_sec_load(kpw_file* this, kpw_idx_it *it, uint64* size) {
	uint64 sz;
	void* buf;
	if (!kpw_idx_fileh(this, *it, &sz)) return NULL;
	buf = pz_malloc(sz);
	sz = fread(buf, 1, sz, this->f);
	if (size) *size = sz;
	return buf;
}

void kpw_sec_new(kpw_file* this, uint type) {
	// Looks for an empty entry
	kpw_idx_it it = kpw_sec_find(this, KPW_ID_EMPTY);
	if (KPW_IT_END(it)) { return; /* FIXME: No hay espacio, extender */ }
	this->cur = it.idx;
	this->curi = it.i;
	this->cursz = 0;
	KPW_IT(it)->offset = this->noffset;
	KPW_IT(it)->type = type;
	fseek(this->f, this->noffset, SEEK_SET);
}

void kpw_sec_close(kpw_file* this) {
	this->cur->idx[this->curi].size = this->cursz;
	this->noffset += this->cursz;
	this->cursz = 0;
	kpw_idx_write(this->cur, this->f);
	this->cur = NULL;
}

int kpw_sec_write(kpw_file* this, const void* buf, uint64 size) {
	int wt = fwrite(buf, 1, size, this->f);
	this->cursz += wt;
	return wt;
}
void kpw_sec_write_upd(kpw_file* this) {
	uint64 p = ftell(this->f);
	const kpw_index_ent* e = this->cur->idx+this->curi;
	if (p >= e->offset) this->cursz = p-e->offset;
}

int kpw_sec_read(kpw_file* this, void* buf, uint64 size) {
	int rd = fread(buf, 1, size, this->f);
	this->cursz += rd;
	return rd;
}

void kpw_sec_delete(kpw_file* this, const kpw_idx_it it) {
	if (KPW_IT_END(it)) return;
	kpw_index_ent* e = KPW_IT(it);
	if (this->noffset == e->offset + e->size) {
		this->noffset = e->offset; // TODO: achicar noffest si quedaron huecos
	}
	e->type = KPW_ID_EMPTY;
	e->size = 0;
	e->type = 0;
	kpw_idx_write(it.idx, this->f);
}

kpw_idx_it kpw_sec_find(const kpw_file* this, uint type) {
	kpw_index *p;
	uint i = 0, sn = 0;
	for (p = this->idx; p != NULL; p = p->next) {
		forn(i, p->zidx) if (p->idx[i].type == type) break;
		if (i < p->zidx && p->zidx) break;
		sn += p->zidx;
	}
	if (!p) sn = i = 0;
	return (kpw_idx_it){p, i, sn};
}

FILE* kpw_idx_fileh(kpw_file* this, const kpw_idx_it it, uint64* size) {
	if (KPW_IT_END(it)) return NULL;
	fseek(this->f, KPW_IT(it)->offset, SEEK_SET);
	if (size) *size = KPW_IT(it)->size;
	return this->f;
}

FILE* kpw_sec_fileh(kpw_file* this, uint type, uint64* size) {
	kpw_idx_it it = kpw_sec_find(this, type);
	return kpw_idx_fileh(this, it, size);
}

bool kpw_sec_exists(kpw_file* this, uint type, uint64* size) {
	kpw_idx_it it = kpw_sec_find(this, type);
	if (KPW_IT_END(it)) return FALSE;
	if (size) *size = KPW_IT(it)->size;
	return TRUE;
}

void* kpw_sec_load(kpw_file* this, uint type, uint64* size) {
	uint64 sz;
	void* buf;
	if (!kpw_sec_fileh(this, type, &sz)) return NULL;
	buf = pz_malloc(sz);
	sz = fread(buf, 1, sz, this->f);
	if (size) *size = sz;
	return buf;
}

uint64 kpw_sec_load_into(kpw_file* this, uint type, void* buf, uint64 size) {
	uint64 realsz, toread;
	if (!kpw_sec_fileh(this, type, &realsz)) return 0;
	toread = (realsz < size)?realsz:size;
	if (fread(buf, 1, toread, this->f) < toread) perror("WARNING reading less bytes from kpw section");
	return realsz;
}

uint64 kpw_sec_save_from(kpw_file* this, uint type, void* buf, uint64 size) {
	kpw_idx_it it = kpw_sec_find(this, type);
	if (KPW_IT_END(it)) {
		kpw_sec_new(this, type);
		kpw_sec_write(this, buf, size);
		kpw_sec_close(this);
		return size;
	}
	const kpw_index_ent* const e = KPW_IT(it);
	uint64 towrite = _min(e->size, size);
	fseek(this->f, e->offset, SEEK_SET);
	if (fwrite(buf, 1, towrite, this->f) < towrite) perror("WARNING writing less bytes to kpw section");
	return e->size;
}

void kpw_show_sec(int i, kpw_index_ent* e) {
	if (e->type == KPW_ID_EMPTY) {
		fprintf(stderr, "sec %2u:\ttyp=%8s\n", i, KPW_STRID(e->type));
	} else {
		fprintf(stderr, "sec %2u:\ttyp=%8s\toffset=%8llxh,\tsize=%8llu\n",
				i, KPW_STRID(e->type), e->offset, e->size);
	}
}

