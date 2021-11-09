#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>

#ifdef GZIP
#include "zlib.h"
#endif

#include "common.h"
#include "macros.h"

#include "kpw_check.h"
#include "kpwapi.h"

extern kpw_header kpw_def;
kpw_index* kpw_idx_alloc(uint sz);

#define Report(...) fprintf(stderr, __VA_ARGS__);
#define Fail(...) do { \
		fprintf(stderr, "INVALID KPW FILE\n\t"); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
		if (errno) {\
			fprintf(stderr, "\t"); \
			perror(NULL); \
		} \
		CLEANUP; \
		return FALSE; \
	} while (FALSE);

static bool check_little_endian() {
	int a = 1;
	char *b = (char *)&a;
	return b[0];
}

#define Write(_File, _Size, _Buf) \
	if (fix && fwrite(_Buf, 1, _Size, _File) != _Size) { \
		Fail("Cannot write to file."); \
	}

bool patch(FILE *g, uint64 pos, uint64 size, void *buf) {
	uint64 old = ftell(g);
	fseek(g, pos, SEEK_SET);
	if (fwrite(buf, 1, size, g) != size) return FALSE;
	fseek(g, old, SEEK_SET);
	return TRUE;
}

#define Patch(_File, _Pos, _Size, _Buf) \
	if (fix && !patch(_File, _Pos, _Size, _Buf)) { \
		Fail("Cannot patch file"); \
	}

/* Fix the STATS section */
#define CLEANUP {}
static bool kpw_check_fix_stats(kpw_file *kpwf, bool fix, uint64 soff, uint64 ssize) {
	ppz_stats _data;
	FILE *f = kpwf->f;

	if (ssize > sizeof(ppz_stats)) {
		Fail("DATA section bigger than expected.");
	} else if (ssize < sizeof(ppz_stats)) {
		Fail("DATA section smaller than expected.");
	}

	fseek(f, SEEK_SET, soff);
	if (fread(&_data, 1, sizeof(ppz_stats), f) != ssize) {
		Fail("Cannot read DATA section.");
	}

	if (kpwf->swap_endian) {
		KPW_fix_uint(_data.rev);
		KPW_fix_uint(_data.long_source_a);
		KPW_fix_uint(_data.long_source_b);
		KPW_fix_uint(_data.long_input_a);
		KPW_fix_uint(_data.long_input_b);
		KPW_fix_uint(_data.n);
		KPW_fix_uint(_data.nlzw);
		KPW_fix_uint(_data.zero_mtf);
		KPW_fix_uint(_data.nzz_est);
		KPW_fix_uint(_data.long_output);
		KPW_fix_uint(_data.est_size);
		KPW_fix_uint(_data.mtf_entropy);
		KPW_fix_uint(_data.huf_len);
		KPW_fix_uint(_data.huf_tbl_bit);
		KPW_fix_uint(_data.lzw_codes);
		KPW_fix_uint64(_data.lpat_size);
		KPW_fix_uint64(_data.lpat3_size);
		/* filename_a filename_b hostname */
		KPW_fix_uint(_data.cnt_A);
		KPW_fix_uint(_data.cnt_C);
		KPW_fix_uint(_data.cnt_G);
		KPW_fix_uint(_data.cnt_T);
		KPW_fix_uint(_data.cnt_N);
		KPW_fix_uint(_data.N_blocks);
		KPW_fix_uint(_data.time_bwt);
		KPW_fix_uint(_data.time_mtf);
		KPW_fix_uint(_data.time_lzw);
		KPW_fix_uint(_data.time_mrs);
		KPW_fix_uint(_data.time_huf);
		KPW_fix_uint(_data.time_longp);
		KPW_fix_uint(_data.time_tot);
		KPW_fix_uint64(_data.lpat3dl0_size);
	}
	if (fix) {
		Patch(f, soff, sizeof(ppz_stats), &_data);
	}
	return TRUE;
}
#undef CLEANUP

/* Fix the TRAC section */
#define CLEANUP { if (mem) pz_free(mem); if (trac_buf) pz_free(trac_buf); if (data) pz_free(data); }
static bool kpw_check_fix_trac(kpw_file *kpwf, bool fix, uint64 soff, uint64 ssize,
						uint64 *dest_pos, uint64 *dest_size) {

	FILE *f = kpwf->f;
	kpw_trac_head *mem = NULL;
	uchar *trac_buf = NULL;
	uchar *data = NULL;

	if (ssize < sizeof(kpw_trac_head)) {
		Fail("TRAC section smaller than expected");
	}

	mem = pz_malloc(ssize);
	if (!mem) { Fail("TRAC section too big."); }

	fseek(f, SEEK_SET, soff);
	uint64 input_len = fread(mem, 1, ssize, f);
	if (input_len < ssize) { Fail("Cannot read TRAC section."); }
	input_len -= sizeof(kpw_trac_head);

	if (kpwf->swap_endian) {
		KPW_fix_uint(mem->flags);
		KPW_fix_uint(mem->trac_size);
		KPW_fix_uint(mem->trac_middle);
	}

	uint flags = mem->flags;
	uint trac_size = mem->trac_size;
	uint trac_middle = mem->trac_middle;

	uint final_len = input_len;
	uchar *final_data = mem->data;

	if (kpwf->swap_endian) {
		/* Uncompress (if needed) */
		int res;
		unsigned long uncompressed_len = trac_size * 2*sizeof(uint);
		trac_buf = (uchar *)pz_malloc(uncompressed_len);
		if (flags & KPW_FLAGS_GZ) {
			res = uncompress(trac_buf, &uncompressed_len, mem->data, input_len);
			if (res != Z_OK || uncompressed_len != trac_size * 2*sizeof(uint)) {
				Fail("Error uncompressing TRAC_DATA input: %u", res);
			}
		} else {
			memcpy(trac_buf, mem->data, uncompressed_len);
		}

		pz_free(mem);
		mem = NULL;

		/* Fix endian */
		uint64 j;
		forn (j, 2 * trac_size)
			KPW_fix_uint(((uint *)trac_buf)[j]);

		/* Compress */
		unsigned long compressed_len = compressBound(uncompressed_len);
		compressed_len = (compressed_len+7) & ~(uint64)8;
		flags |= KPW_FLAGS_GZ;
		data = (uchar *)pz_malloc(compressed_len);
		res = compress2(data, &compressed_len, trac_buf, uncompressed_len, PZ_GZ_LEVEL);
		if (res != Z_OK || compressed_len > uncompressed_len) {
			/* Abort use of gz */
			pz_free(data);
			data = NULL;
			flags &= ~KPW_FLAGS_GZ;
			final_len = uncompressed_len;
			final_data = trac_buf;
		} else {
			final_len = compressed_len;
			final_data = data;
		}
	}

	*dest_size = 3 * sizeof(uint) + final_len;

	/* Don't leave a hole if the new size is smaller
	 * than the old size */
	if (*dest_size <= ssize) *dest_pos = soff;

	if (fix) {
		uint64 patch_pos = *dest_pos;
		Patch(f, patch_pos, sizeof(uint), &flags);
		patch_pos += sizeof(uint);
		Patch(f, patch_pos, sizeof(uint), &trac_size);
		patch_pos += sizeof(uint);
		Patch(f, patch_pos, sizeof(uint), &trac_middle);
		patch_pos += sizeof(uint);
		Patch(f, patch_pos, final_len, final_data);
	}

	CLEANUP;
	return TRUE;
}
#undef CLEANUP

/* Fix the BWT section */
#define CLEANUP {}
static bool kpw_check_fix_bwt(kpw_file *kpwf, bool fix, uint64 soff, uint64 ssize) {
	FILE *f = kpwf->f;

	fseek(f, soff, SEEK_SET);
	/* Fix BWT header */
	kpw_bwt_head bwt_head;
	if (fread(&bwt_head, 1, sizeof(bwt_head), f) != sizeof(bwt_head)) {
		Fail("Cannot read BWT header.");
	}
	if (kpwf->swap_endian) {
		KPW_fix_uint(bwt_head.flags);
		KPW_fix_uint64(bwt_head.n);
		KPW_fix_uint64(bwt_head.prim);
	}
	if (fix) {
		Patch(f, soff, sizeof(bwt_head), &bwt_head);
	}
	return TRUE;
}
#undef CLEANUP

/* Fix the PATT section */
#define CLEANUP {}
static bool kpw_check_fix_patt(kpw_file *kpwf, bool fix, uint64 soff, uint64 ssize) {
	FILE *f = kpwf->f;

	uint64 total_bytes = ssize;
	uint dat[3];

	uint size_bytes;
	uint64 patch_pos = soff;

	/**** For each different length ****/
	while (total_bytes > 0) {
		/* Read subsection header */
		fseek(f, patch_pos, SEEK_SET);
		if (fread(dat, 1, sizeof(dat), f) != sizeof(dat))
			Fail("Cannot read PATT section.");

		if (kpwf->swap_endian) {
			KPW_fix_uint(dat[0]);
			KPW_fix_uint(dat[1]);
			KPW_fix_uint(dat[2]);
		}
		
		size_bytes = dat[2];
		if (total_bytes < size_bytes) {
			Fail("ERROR: total_bytes < size_bytes. Invalid PATT section");
		}
		if (size_bytes < sizeof(dat)) {
			Fail("ERROR: size_bytes must be at least %lu bytes. Invalid PATT section", (long unsigned)sizeof(dat));
		}

		if (fix) {
			Patch(f, patch_pos, sizeof(dat), dat);
		}
		total_bytes -= size_bytes;
		/* Skip to the next section */
		patch_pos += size_bytes;
	}
	return TRUE;
}
#undef CLEANUP

/* Check and fix a whole section, including each of its entries */
#define CLEANUP { if (res) pz_free(res); }
static bool kpw_check_fix_idx(kpw_file *kpwf,
		uint64 *fsize, bool fix, uint64 *next_offset) {
	kpw_index *res = NULL;
	FILE *f = kpwf->f;

	uint zidx;
	uint64 off;

	uint64 patch_pos = ftell(f);
	/**** Section header ****/
	if (!fread(&zidx, sizeof(zidx), 1, f)) {
		Fail("Cannot read index <zidx>.");
	}
	if (!fread(&off, sizeof(off), 1, f)) {
		Fail("Cannot read index <off>.");
	}

	/* XXX: kludge. */
	/* Swap endian when the number of sections is "too big" */
	if (kpwf->swap_endian) {
		/* We're already swapping endian */
		KPW_fix_uint(zidx);
		KPW_fix_uint64(off);
	}
 	if (zidx & 0xffff0000) {
		if (kpwf->swap_endian) {
			/* zidx is too big even if we are swapping endian */
			Fail("<zidx> occurrences too big both in little "
				" and big endian. Inconsistent usage of endian.");
		} else {
			/* Let's swap endian! */
			kpwf->swap_endian = 1;

			fprintf(stderr, "WARNING: <zidx> too big. Swapping endian.\n");

			KPW_fix_uint(zidx);
			KPW_fix_uint64(off);
		}
	}
	if (off >= *fsize) {
		Fail("Offset out of bounds: %llu (filesize: %llu).",
			off, *fsize);
	}
	/* Patch it */
	if (fix) {
		Patch(f, patch_pos, sizeof(zidx), &zidx);
		patch_pos += sizeof(zidx);
		Patch(f, patch_pos, sizeof(off), &off);
		patch_pos += sizeof(off);
	}

	/**** Entries ****/
	res = kpw_idx_alloc(zidx);
	res->offset = off;
	if (!fread(res->idx, sizeof(kpw_index_ent), zidx, f)) {
		Fail("Cannot read entry header.");
	}

	uint i;

	/* Fix endian of the entries */
	if (kpwf->swap_endian) {
		forn(i, zidx) {
			KPW_fix_uint64(res->idx[i].offset);
			KPW_fix_uint64(res->idx[i].size);
			KPW_fix_uint(res->idx[i].type);
		}
	}

	/* Check and fix each entry */
	forn(i, zidx) {
		uint64 soff = res->idx[i].offset;
		uint64 ssize = res->idx[i].size;
		uint stype = res->idx[i].type;

		kpw_show_sec(i, &res->idx[i]);
		if (stype == KPW_ID_EMPTY) continue; /* Skip empty section */
		if (stype >= KPW_MAX_SECTION_ID) {
			Fail("sec %2u of invalid type: %2u", i, stype);
		}

		/* Check bounds */
		if (soff > *fsize || soff + ssize > *fsize) {
			Fail("sec %2u out of bounds: "
				"start=%8llx, end=%8llx (filesize: %8llx).",
				i, soff, soff + ssize - 1, *fsize);
		}

		/* Go ahead and check/fix each section */

		switch (stype) {
		case KPW_ID_STATS:
			if (!kpw_check_fix_stats(kpwf, fix, soff, ssize)) {
				CLEANUP;
				return FALSE;
			}
			break;
		case KPW_ID_TRAC: {
				/* The TRAC section could be compressed, so it might
				 * grow after fixing the endianness.
				 *
				 * When it grows, we leave a hole (with garbage) in the
				 * old TRAC section. Also, we update the section header
				 * to point to the newly created TRAC section, which
				 * is put at the end of the file.
				 */
				uint64 dest_pos = *fsize;
				uint64 dest_size;
				if (!kpw_check_fix_trac(kpwf, fix, soff, ssize, &dest_pos, &dest_size)) {
					CLEANUP;
					return FALSE;
				}
				ssize = dest_size;
				if (soff != dest_pos) {
					/* There's a hole */
					soff = dest_pos;
					*fsize += ssize;
				}
			}
			break;
		case KPW_ID_BWT:
			if (!kpw_check_fix_bwt(kpwf, fix, soff, ssize)) {
				CLEANUP;
				return FALSE;
			}
			break;
		case KPW_ID_PATT:
			if (!kpw_check_fix_patt(kpwf, fix, soff, ssize)) {
				CLEANUP;
				return FALSE;
			}
			break;
		default:
			fprintf(stderr, "WARNING: unknown type of section: %2u\n", stype);
			break;
		}

		res->idx[i].offset = soff;
		res->idx[i].size = ssize;
	}
	Patch(f, patch_pos, sizeof(kpw_index_ent) * zidx, res->idx);

	CLEANUP;

	if (off != 0 && ftell(f) != off) {
		fprintf(stderr, "WARNING: offset doesn't match with real offset");
	}

	*next_offset = off;
	return TRUE;
}
#undef CLEANUP

#define CLEANUP { if (f) fclose(f); }
bool kpw_check_fix(char *fn, bool fix) {
	kpw_file kpwf;
	FILE *f = NULL;

	/**** Open files ****/
	Report("* Opening file.\n");
	uint64 fsize = filesize(fn);

	/* If checking, open for read-only */
	f = fopen(fn, (fix ? "rb+" : "r+"));
	if (fsize == -1 || !f) {
		Fail("File doesn't exist");
	}

	kpwf.f = f;
	kpwf.cur = NULL;
	kpwf.swap_endian = FALSE;

	/**** Read/write KPW header ****/
	Report("* Reading header.\n");
	kpw_header hd;
	if (fread(&hd, 1, sizeof(hd), f) != sizeof(hd)) {
		Fail("Cannot read file header");
	}

	if (memcmp(kpw_def.magic, hd.magic, 4)) {
		Fail("Magic number doesn't match");
	}

	/**** Process each section ****/
	Report("* Checking contents.\n");

	uint num_part = 0;
	uint64 next_offset;
	for (;;) {
		Report("* Part #%i.\n", num_part++);
		if (!kpw_check_fix_idx(&kpwf, &fsize, fix, &next_offset)) {
			CLEANUP;
			return FALSE; /* Failure */
		}
		if (!next_offset) break; /* This was the last section */
		if (next_offset >= fsize) {
			Fail("Section offset out of bounds.");
		}
		fseek(f, next_offset, SEEK_SET);
	}

	CLEANUP;

	if (check_little_endian() ^ kpwf.swap_endian)
		fprintf(stderr, "File OK.\n");
	else if (fix)
		fprintf(stderr, "File OK [Fixed endian].\n");
	else
		fprintf(stderr, "File OK [WARNING: big endian].\n");

	return TRUE;
}
#undef CLEANUP
#undef Report
#undef Fail
#undef Sizes
#undef Patch

