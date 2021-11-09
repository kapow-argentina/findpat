#include <endian.h>
#include <string.h>
#include "tipos.h"
#include "libp2zip.h"
#include "common.h"
#include "kpwapi.h"

#include "macros.h"

#ifdef GZIP
#include <zlib.h>
#endif

#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __BIG_ENDIAN
#define KPW_SWAP_ENDIANNESS
#endif
#endif

/*** STAT section ***/

void endian_swap_stats(ppz_stats* data) {
	endian_swap_32(&data->rev);
	endian_swap_32(&data->long_source_a);
	endian_swap_32(&data->long_source_b);
	endian_swap_32(&data->long_input_a);
	endian_swap_32(&data->long_input_b);
	endian_swap_32(&data->n);
	endian_swap_32(&data->nlzw);
	endian_swap_32(&data->zero_mtf);
	endian_swap_32(&data->nzz_est);
	endian_swap_32(&data->long_output);
	endian_swap_32(&data->est_size);
	endian_swap_32(&data->mtf_entropy);
	endian_swap_32(&data->huf_len);
	endian_swap_32(&data->huf_tbl_bit);
	endian_swap_32(&data->lzw_codes);
	endian_swap_64(&data->lpat_size);
	endian_swap_64(&data->lpat3_size);
	/* filename_a filename_b hostname */
	endian_swap_32(&data->cnt_A);
	endian_swap_32(&data->cnt_C);
	endian_swap_32(&data->cnt_G);
	endian_swap_32(&data->cnt_T);
	endian_swap_32(&data->cnt_N);
	endian_swap_32(&data->N_blocks);
	endian_swap_32(&data->time_bwt);
	endian_swap_32(&data->time_mtf);
	endian_swap_32(&data->time_lzw);
	endian_swap_32(&data->time_mrs);
	endian_swap_32(&data->time_huf);
	endian_swap_32(&data->time_longp);
	endian_swap_32(&data->time_tot);
	endian_swap_64(&data->lpat3dl0_size);
	endian_swap_32(&data->time_mmrs);
}

bool kpw_load_stats(kpw_file* kpwf, ppz_stats* data, uint size) {
	if (!kpwf) return FALSE;
	if (!kpw_sec_load_into(kpwf, KPW_ID_STATS, data, size)) return FALSE;

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_stats(data);
#endif

	return TRUE;
}

void kpw_save_stats(kpw_file* kpwf, ppz_stats* data) {
	if (!kpwf) return;
#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_stats(data);
#endif

	/* Borra el STATS actual si es mas viejo */
	kpw_idx_it it = kpw_sec_find(kpwf, KPW_ID_STATS);
	if (!KPW_IT_END(it) && KPW_IT(it)->size != sizeof(*data)) {
		kpw_sec_delete(kpwf, it);
	}
	kpw_sec_save_from(kpwf, KPW_ID_STATS, data, sizeof(*data));

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_stats(data);
#endif
}

/*** TRAC section ***/

void endian_swap_trac(uint* trac_buf, uint t_size) {
	int j;
	forn(j, 2 * t_size) endian_swap_32(trac_buf+j);
}

uint* kpw_load_trac(kpw_file* kpwf, uint* t_size, uint* t_middle) {
	uint64 len;
	kpw_trac_head *mem;
	uint *trac_buf;
	uint res;

	if (!kpwf) return NULL;
	len = 0;
	mem = (kpw_trac_head*)kpw_sec_load(kpwf, KPW_ID_TRAC, &len);
	if (!mem) return NULL;
	if (len < sizeof(kpw_trac_head)) {
		pz_free(mem);
		return FALSE;
	}
	len -= sizeof(kpw_trac_head);

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(&mem->flags);
	endian_swap_32(&mem->trac_size);
	endian_swap_32(&mem->trac_middle);
#endif

	unsigned long lenO = mem->trac_size * 2*sizeof(uint);
	trac_buf = (uint*)pz_malloc(lenO);
	if (mem->flags & KPW_FLAGS_GZ) {
		res = uncompress((uchar*)trac_buf, &lenO, mem->data, len);
		if (res != Z_OK || lenO != mem->trac_size * 2*sizeof(uint)) {
			ppz_error("Error uncompressing TRAC_DATA input: %u", res);
			pz_free(mem); pz_free(trac_buf); return FALSE;
		}
	} else {
		memcpy(trac_buf, mem->data, lenO);
	}

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_trac(trac_buf, mem->trac_size);
#endif

	if (t_size) *t_size = mem->trac_size;
	if (t_middle) *t_middle = mem->trac_middle;
	pz_free(mem);
	return trac_buf;
}

bool kpw_save_trac(kpw_file* kpwf, uint* trac_buf, uint t_size, uint t_middle) {
	kpw_trac_head mem;
	void *data;
	int res, use_gz = 1;
	unsigned long len = 0, datalen = 0;
	if (!kpwf) return FALSE;
	if (kpw_sec_exists(kpwf, KPW_ID_TRAC, NULL)) {
		ppz_error("%s", "TRAC section already exists");
		return FALSE;
	}

	mem.flags = 0;
	mem.trac_size = t_size;
	mem.trac_middle = t_middle;
	datalen = mem.trac_size * 2 * sizeof(uint);

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_trac(trac_buf, t_size);
#endif

	kpw_sec_new(kpwf, KPW_ID_TRAC);
	if (use_gz) {
		mem.flags |= KPW_FLAGS_GZ;
		len = compressBound(datalen); len = (len+7) & ~(uint64)8;
		data = (uchar*)pz_malloc(len);
//		memset(data, 0x00, len);
		res = compress2(data, &len, (uchar*)trac_buf, datalen, PZ_GZ_LEVEL);
		if (res != Z_OK || len > datalen) { /* Abort use of gz */
			use_gz = 0;
			pz_free(data);
			mem.flags &= ~KPW_FLAGS_GZ;
		}
	}
	if (!use_gz) {
		len = datalen;
		data = trac_buf;
	}

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(&mem.flags);
	endian_swap_32(&mem.trac_size);
	endian_swap_32(&mem.trac_middle);
#endif

	kpw_sec_write(kpwf, &mem, sizeof(mem));
	kpw_sec_write(kpwf, data, len);
	kpw_sec_close(kpwf);
	if (use_gz) pz_free(data);

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_trac(trac_buf, t_size);
#endif

	return TRUE;
}

/*** Section BWT ***/
uchar* kpw_load_bwt(kpw_file* kpwf, uint* len, uint* prim) {
	uint64 lenI;
	kpw_bwt_head *mem;
	uchar *bwto;
	int res;
	uint i;
	bool headfix = FALSE;

	if (!kpwf) return 0;
	lenI = 0;
	mem = (kpw_bwt_head*)kpw_sec_load(kpwf, KPW_ID_BWT, &lenI);
	if (!mem) return NULL;
	if (lenI < sizeof(kpw_bwt_head)) {
		pz_free(mem);
		return NULL;
	}
	lenI -= sizeof(kpw_bwt_head);

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(&mem->flags);
	endian_swap_64(&mem->n);
	endian_swap_64(&mem->prim);
#endif

	if (len && *len && *len != mem->n) {
		ppz_error("BWT header seems to be invalid... recalculating%c", '\n');
		/*
		ppz_error("Previous input size mismatch%c", '\n');
		pz_free(mem);
		return NULL;
		*/
		mem->n = *len;
		mem->flags = KPW_FLAGS_GZ;
		mem->prim = -1;
		headfix = TRUE;
	}

	bwto = (uchar*)pz_malloc(mem->n * sizeof(uchar));
	if (mem->flags & KPW_FLAGS_GZ) {
		unsigned long lenO = mem->n;
		res = uncompress(bwto, &lenO, mem->data, lenI);
		if (res != Z_OK || lenO != mem->n) {
			ppz_error("Error uncompressing BWT input: %s", zError(res));
			pz_free(mem); pz_free(bwto); return NULL;
		}
	} else {
		memcpy(bwto, mem->data, mem->n);
	}

	/* Calcula prim */
	if (headfix) {
		forn(i, mem->n) if (bwto[i] == 0xff) { mem->prim = i; break; }
	}

	if (prim) *prim = mem->prim;
	if (len) *len = mem->n;
	
	if (headfix) {
#ifdef KPW_SWAP_ENDIANNESS
		endian_swap_32(&mem->flags);
		endian_swap_64(&mem->n);
		endian_swap_64(&mem->prim);
#endif
		kpw_sec_save_from(kpwf, KPW_ID_BWT, mem, sizeof(kpw_bwt_head));
	}
	pz_free(mem);
	return bwto;
}

bool kpw_save_bwt(kpw_file* kpwf, uchar* bwto, uint n, uint prim) {
	kpw_bwt_head mem;
	uchar *data;
	int res, use_gz = 1;
	unsigned long len = 0;
	if (!kpwf) return FALSE;

//	bwto = this->bwto?this->bwto:(uchar*)this->p;
	if (kpw_sec_exists(kpwf, KPW_ID_BWT, NULL)) {
		ppz_error("%s", "BWT section already exists");
		return FALSE;
	}

	mem.prim = prim;
	mem.n = n;
	mem.flags = 0;
	kpw_sec_new(kpwf, KPW_ID_BWT);
	if (use_gz) {
		mem.flags |= KPW_FLAGS_GZ;
		len = compressBound(mem.n); len = (len+7) & ~(uint64)8;
		data = (uchar*)pz_malloc(len * sizeof(uchar));
//		memset(data, 0x00, len);
		res = compress2(data, &len, bwto, mem.n, PZ_GZ_LEVEL);
		if (res != Z_OK || len > mem.n) { /* Abort use of gz */
			use_gz = 0;
			pz_free(data);
			mem.flags &= ~KPW_FLAGS_GZ;
		}
	}
	if (!use_gz) {
		len = mem.n;
		data = bwto;
	}

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(&mem.flags);
	endian_swap_64(&mem.n);
	endian_swap_64(&mem.prim);
#endif

	kpw_sec_write(kpwf, &mem, sizeof(mem));
	kpw_sec_write(kpwf, data, len);
	kpw_sec_close(kpwf);
	if (use_gz) pz_free(data);
	return TRUE;
}

/*** Section PATT ***/

bool kpw_sec_patt_subsec_open(kpw_file* kpwf) {
	uint dat[3];
	dat[0] = KPW_PATT_INVALID;
	dat[1] = 0; /* Quantity filled at the end */
	dat[2] = 0; /* Size of the length part (in bytes), including header */
#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(dat+0);
	endian_swap_32(dat+1);
	endian_swap_32(dat+2);
#endif
	return kpw_sec_write(kpwf, dat, sizeof(dat));
}

#define bi2by(X) (((X)+7)/8)
bool kpw_sec_patt_subsec_close(kpw_file* kpwf, uint subsec_off, uint cur_length, uint cur_cant) {
	uint dat[3];
	uint64 current = ftell(kpwf->f);
//	uint64 current = subsec_off + sizeof(dat) + bi2by(this->obs.bits);
	fseek(kpwf->f, subsec_off, SEEK_SET);
	dat[0] = cur_length;
	dat[1] = cur_cant;
	dat[2] = current - subsec_off;

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(dat+0);
	endian_swap_32(dat+1);
	endian_swap_32(dat+2);
#endif

	int ok = fwrite(dat, 1, sizeof(dat), kpwf->f);
	fseek(kpwf->f, current, SEEK_SET);
	return !!ok;
}

/*** Section LCP ***/

void endian_swap_array32(uint* a, uint n) {
	int i;
	forn(i, n) endian_swap_32(&a[i]);
}

uint *kpw_load_lcp(kpw_file* kpwf, uint64* len) {
	uint64 lenI;
	kpw_lcp_head *mem;
	uint *h;
	int res;

	if (!kpwf) return 0;
	lenI = 0;
	mem = (kpw_lcp_head*)kpw_sec_load(kpwf, KPW_ID_LCP, &lenI);
	if (!mem) return NULL;
	if (lenI < sizeof(kpw_lcp_head)) {
		pz_free(mem);
		return NULL;
	}
	lenI -= sizeof(kpw_lcp_head);

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(&mem->flags);
	endian_swap_64(&mem->n);
#endif

	h = (uint*)pz_malloc(mem->n);
	if (mem->flags & KPW_FLAGS_GZ) {
		unsigned long lenO = mem->n;
		res = uncompress((uchar *)h, &lenO, mem->data, lenI);
		if (res != Z_OK || lenO != mem->n) {
			ppz_error("Error uncompressing LCP input: %s", zError(res));
			pz_free(mem); pz_free(h); return NULL;
		}
	} else {
		memcpy(h, mem->data, mem->n);
	}
	
#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_array32(h, mem->n);
#endif

	if (len) *len = mem->n;
	
	pz_free(mem);
	return h;
}

bool kpw_save_lcp(kpw_file* kpwf, uint* h, uint n) {
	void *data;
	kpw_lcp_head mem;
	int res, use_gz = 1;
	unsigned long len = 0;
	if (!kpwf) return FALSE;

	mem.n = n * sizeof(uint);
	mem.flags = 0;
	if (kpw_sec_exists(kpwf, KPW_ID_LCP, NULL)) {
		ppz_error("%s", "LCP section already exists");
		return FALSE;
	}

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_array32(h, n);
#endif

	kpw_sec_new(kpwf, KPW_ID_LCP);
	if (use_gz) {
		mem.flags |= KPW_FLAGS_GZ;
		len = compressBound(mem.n); len = (len+7) & ~(uint64)8;
		data = (uchar*)pz_malloc(len * sizeof(uchar));
//		memset(data, 0x00, len);
		res = compress2(data, &len, (uchar*)h, mem.n, PZ_GZ_LEVEL);
		if (res != Z_OK || len > mem.n) { /* Abort use of gz */
			use_gz = 0;
			pz_free(data);
			mem.flags &= ~KPW_FLAGS_GZ;
		}
	}
	if (!use_gz) {
		len = mem.n;
		data = h;
	}

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(&mem.flags);
	endian_swap_64(&mem.n);
#endif

	kpw_sec_write(kpwf, &mem, sizeof(mem));
	kpw_sec_write(kpwf, data, len);
	kpw_sec_close(kpwf);
	if (use_gz) pz_free(data);

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_array32(h, n);
#endif
	return TRUE;
}

/* LCPset */

uchar *kpw_load_lcpset(kpw_file* kpwf, uint64* len) {
	uint64 lenI;
	kpw_lcp_head *mem;
	uchar *hset;
	int res;

	if (!kpwf) return 0;
	lenI = 0;
	mem = (kpw_lcp_head*)kpw_sec_load(kpwf, KPW_ID_LCPS, &lenI);
	if (!mem) return NULL;
	if (lenI < sizeof(kpw_lcp_head)) {
		pz_free(mem);
		return NULL;
	}
	lenI -= sizeof(kpw_lcp_head);

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(&mem->flags);
	endian_swap_64(&mem->n);
#endif

	uint64 n = mem->n;
	uint msize = (n-1+7)/8;

	uchar* temp = NULL;
	if (mem->flags & KPW_FLAGS_GZ) {
		temp = (uchar*)pz_malloc(msize+sizeof(uint));
		unsigned long lenO = msize+4;
		res = uncompress((uchar *)temp, &lenO, mem->data, lenI);
		if (res != Z_OK) {
			ppz_error("Error uncompressing LCP-SET input: %s", zError(res));
			pz_free(mem); pz_free(temp); return NULL;
		}
	} else {
		temp = mem->data;
	}

	hset = (uchar*)pz_malloc(msize); /* Reserve space for returned lcpset */
	uchar* hres = hset;
	uint i=0, x;
	uint* p = (uint*) temp;
	for(x=0; (i < n-1) && *p; ++x, ++p) {
		uint q = *p;
		for ( ; *p; ++i,(*p)--) {
			if (i%8 == 0) *hres = 0;
			if (i && *p == q) *hres |= 1 << (i%8);
			if (i%8 == 7) hres++;
		}
	}
	++p;
	if (i < n-1) memcpy(hres, p, msize - i/8);

	if (len) *len = n;
	if (mem->flags & KPW_FLAGS_GZ) pz_free(temp);
	pz_free(mem);
	
	return hset;
}

bool kpw_save_lcpset(kpw_file* kpwf, uchar* hset, uint n) {
	void *data;
	kpw_lcp_head mem;
	int res, use_gz = 1;
	unsigned long len = 0;
	if (!kpwf) return FALSE;

	mem.n = n;
	uint msize = (n-1+7)/8;
	mem.flags = 0;
	if (kpw_sec_exists(kpwf, KPW_ID_LCPS, NULL)) {
		ppz_error("%s", "LCPS section already exists");
		return FALSE;
	}

//	DBGu(mem.n);

	uchar *ohset = hset;
	// Saves the firsts numbers in ints and the lasts in bits
	uint* bits = (uint*)pz_malloc(msize + sizeof(uint));
	uint* chset = bits;
	uint64 mi=1, mz=1, mx=0, z=0;
	uint i, vl = 0, lvl=-1, cnt=0, li=0;
	forn(i, n) {
		if (i<n-1) vl += (*hset >> (i%8)) & 1;
		cnt++;
		if ((i == n-1) || (i && vl != lvl)) {
			z = (uint64)vl*32;
//			fprintf(stderr, "lvl=%3d \t[li,i)=[%3d,%3d)  cst=%.3f\n", lvl, li, i, (float)z/i);
			if (z * mi <= mz * i) { mz = z; mi = i; mx=vl; }
			if (vl < msize/4) *(chset++) = i-li;
			li = i;
			cnt = 0;
		}
		lvl = vl;
		if (i%8 == 7) hset++;
	}
	if (!mx) { mi = 0; mz = 0; }

//	DBGu(mx); DBGu(mi); DBGu(mz);
#ifdef KPW_SWAP_ENDIANNESS
	forn(i, mx) endian_swap_32(bits+i);
#endif

	chset = bits+mx;
	uchar *cchset = (uchar*)chset;
	/* Put the remaining bits */
	if (mi < n-1) {
		*(chset++) = 0; /* no need to swap the value 0 */
		memcpy(chset, ohset + mi/8, msize - mi/8);
		cchset = ((uchar*)chset) + msize - mi/8;
	}
	
	msize = cchset - (uchar*)bits;
//	DBGu(msize);
	
	kpw_sec_new(kpwf, KPW_ID_LCPS);
	if (use_gz) {
		mem.flags |= KPW_FLAGS_GZ;
		len = compressBound(msize); len = (len+7) & ~(uint64)8;
		data = (uchar*)pz_malloc(len * sizeof(uchar));
		res = compress2(data, &len, (uchar*)bits, msize, PZ_GZ_LEVEL);
		if (res != Z_OK || len >= msize) { /* Abort use of gz */
			use_gz = 0;
			pz_free(data);
			mem.flags &= ~KPW_FLAGS_GZ;
		}
	}
	if (!use_gz) {
		len = msize;
		data = bits;
	}

#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(&mem.flags);
	endian_swap_64(&mem.n);
#endif

	kpw_sec_write(kpwf, &mem, sizeof(mem));
	kpw_sec_write(kpwf, data, len);
	kpw_sec_close(kpwf);
//	DBGu(use_gz);
	if (use_gz) pz_free(data);
	pz_free(bits);

	return TRUE;
}


/* KPW - iterator for PATT section */

void ppz_kpw_patt_it_create(ppz_status *this, ppz_kpw_patt_it *it) {
	it->data_encr = bneed(this->data.n - 1);
	it->kpwf = this->kpwf;
	it->inf = kpw_sec_fileh(this->kpwf, KPW_ID_PATT, &it->tot_bytes);
	it->current_length = 0;
}

uint ppz_kpw_patt_next_subsection(ppz_kpw_patt_it *it) {
	uint dat[3];
	uint cant_patterns;
	uint size_bytes;

	/* End of section found */
	if (it->tot_bytes == 0)
		return 0;

	/* Destroy the ibitstream for the previous subsection if there was one */
	if (it->current_length > 0)
		ibs_destroy(&it->ibs);

	/* Read the header for this subsection */
	kpw_sec_read(it->kpwf, dat, sizeof(dat));
#ifdef KPW_SWAP_ENDIANNESS
	endian_swap_32(dat+0);
	endian_swap_32(dat+1);
	endian_swap_32(dat+2);
#endif
	it->current_length = dat[0];
	cant_patterns = dat[1];
	size_bytes = dat[2];
	it->tot_bytes -= size_bytes;

	/* Initialize the ibitstream for this subsection */
	ibs_create(&it->ibs, it->inf, size_bytes - sizeof(dat));
	return cant_patterns;
}

uint ppz_kpw_patt_next_pattern(ppz_kpw_patt_it *it) {
	/* Index of r[] in in data_encr fixed bits (same bits as n) */
	it->pattern_index = ibs_read_uint(&it->ibs, it->data_encr);

	/* Number of occurrences of this pattern */
	/*return ibs_read_bbpb(&it->ibs) + 2;*/
	return ibs_read_bbpb(&it->ibs);
}

void ppz_kpw_patt_it_destroy(ppz_kpw_patt_it *it) {
	if (it->current_length > 0)
		ibs_destroy(&it->ibs);
}

void ppz_kpw_iterate(ppz_status *this, ppz_kpw_patt_callback callback) {
	ppz_kpw_patt_it patt_it;
	ppz_kpw_patt_it_create(this, &patt_it);
	uint cant, len, occs, idx;
	while ((cant = ppz_kpw_patt_next_subsection(&patt_it))) {
		len = patt_it.current_length;
		while (cant--) {
			occs = ppz_kpw_patt_next_pattern(&patt_it);
			idx = patt_it.pattern_index;
			(*callback)(len, occs, idx);
		}
	}
	ppz_kpw_patt_it_destroy(&patt_it);
}

