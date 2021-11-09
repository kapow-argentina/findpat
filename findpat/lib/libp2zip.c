#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <math.h>
#include "libp2zip.h"
#include "tiempos.h"
#include "tipos.h"
#include "bwt.h"
#include "klcp.h"
#include "mtf.h"
#include "common.h"
#include "enc.h"
#include "lzw.h"
#include "arit_enc.h"
#include "huffman.h"
#include "longpat.h"
#include "lcp.h"
#include "bittree.h"
#include "mrs.h"
#include "mrs_nolcp.h"
#include "output_callbacks.h"
#include "bitstream.h"
#include "kpw.h"
#include "kpwapi.h"
#include "mmrs.h"

#include "macros.h"

#ifdef GZIP
#include <zlib.h>
#endif

void ppz_create(ppz_status* this) {
	struct utsname buf;
	this->st = PZ_ST_CREATED;
	this->p = this->r = NULL;
	this->h = NULL;
	this->s = this->bwto = NULL;
	this->lzw_cds = 0;
	this->no = 0;
	this->fa_input = 0;
	this->kpwf = NULL;
	this->update_data = 0;
	#ifdef GZIP
	this->gz_input = 1;
	#endif
	this->use_terminator = this->use_separator = 0;
	memset(&(this->data), 0x00, sizeof(this->data));

	#ifdef SVNREVISION
	this->data.rev = SVNREVISION;
	#endif

	this->trac_positions = 0;
	this->trac_buf = 0;
	this->trac_size = this->trac_middle = 0;

	if (uname(&buf) == -1) { perror("uname"); return; }
	snprintf(this->data.hostname, MAX_FILENAME_LEN, "%s-%s", buf.nodename, buf.machine);
}

void ppz_destroy(ppz_status* this) {
	if (this->kpwf) {
		if (this->update_data) ppz_kpw_save_stats(this);
		kpw_close(this->kpwf); this->kpwf = NULL;
	}
	if (this->s) { pz_free(this->s); this->s = 0; }
	if (this->bwto) { pz_free(this->bwto); this->bwto = 0; }
	if (this->p) { pz_free(this->p); this->p = 0; }
	if (this->h) { pz_free(this->h); this->h = 0; }
	if (this->r) { pz_free(this->r); this->r = 0; }
	if (this->lzw_cds) { pz_free(this->lzw_cds); this->lzw_cds = 0; }
	if (this->trac_buf) { pz_free(this->trac_buf); this->trac_buf = 0; }
}

void ppz_compress_gen(ppz_status* this, void (*bwtf)(uchar*, uint*, uint*, uchar*, uint, uint*)) {
	tiempo t1, t2;
	getTickTime(&t1);

	ppz_bwt_gen(this, bwtf);
//	ppz_longpat(this);

	ppz_mtf(this);
	ppz_mtf_ests(this);

//	ppz_mrs(this, 0, 1);
/*
	ppz_lzw(this);
	ppz_est_size(this);
	ppz_huffman_len(this);
	this->data.long_output = 4 + 4 + this->data.huf_len; // CRC32 + BWT_prim + HUFFMAN_OUPUT
*/

	getTickTime(&t2);
	this->data.time_tot = (uint)(getTimeDiff(t1, t2));
}

void ppz_compress(ppz_status* this) {
	ppz_compress_gen(this, &bwt);
}

void ppz_ocompress(ppz_status* this) {
	ppz_compress_gen(this, &obwt);
}


void ppz_decompress(ppz_status* this);

/* private functions */
#define ppz_ensure(cond, ret) { if (!(cond)) { fprintf(stderr, "%s:%d: Status condition '%s' not meet.\n", __FILE__, __LINE__, #cond); return ret; } }

/** str_right_cpy
 * Copies at most size_t from the end of the src string to dest
 */
void str_right_cpy(char* dest, const char* src, size_t count) {
	size_t l;
	if (!src) { dest[0] = 0; return; }
	l = strlen(src);
	if (l > count-1) src+= l - (count-1);
	strcpy(dest, src);
}

const char* get_filename(const char* fn) {
	const char* p = fn, *lp;
	if (!fn) return fn;
	while ((lp = strchr(p, '/'))) p = lp+1;
	return p;
}

/* pseudo-private members */
int ppz_load_file(ppz_status* this, const char* fn) {
	return ppz_load_file2(this, fn, NULL);
}

#define MAX_FILENAME_GZ 1024
#ifdef GZIP
#define TRY_LOAD_FILE(fn, v, lv) \
	if (this->gz_input) { v = loadStrGzFile(fn, &lv); \
		if (!v) { snprintf(fngz, MAX_FILENAME_GZ, "%s.gz", fn); v = loadStrGzFile(fngz, &lv); } \
	} else v = loadStrFile(fn, &lv);
#else
#define TRY_LOAD_FILE(fn, v, lv) v = loadStrFile(fn, &lv);
#endif

int ppz_load_file2(ppz_status* this, const char* fn1, const char* fn2) {
	uchar *a, *b, *ab, *s;
	uint la, lb, n;
	uint bc[256], i;
	memset(bc, 0x00, sizeof(bc));
#ifdef GZIP
	char fngz[MAX_FILENAME_GZ];
#endif

	ppz_ensure(this->st == PZ_ST_CREATED, 0);

	if (this->kpwf) {
		kpw_idx_it it = kpw_sec_find(this->kpwf, KPW_ID_BWT);
		if (!KPW_IT_END(it)) {
			ppz_error("%s", "Ignoring input files, will use KPW file.");
			if (this->trac_positions) ppz_kpw_load_trac(this);
			this->st = PZ_ST_LOADED;
			return this->data.n;
		}
	}

	if (!fn1) { ppz_error("%s", "Error: Invalid input file (NULL)"); return 0; }

	TRY_LOAD_FILE(fn1, a, la)
	if (!a) { ppz_error("Error loading file: '%s'", fn1); return 0; }

	if (fn2) {
		TRY_LOAD_FILE(fn2, b, lb)
		if (!b) {
			pz_free(a);
			ppz_error("Error loading file: '%s'", fn2); return 0;
		}
	} else {
		lb = 0;
		b = NULL;
	}

	/* Stats */
	str_right_cpy(this->data.filename_a, get_filename(fn1), MAX_FILENAME_LEN);
	str_right_cpy(this->data.filename_b, get_filename(fn2), MAX_FILENAME_LEN);
	this->data.long_input_a = this->data.long_source_a = la;
	this->data.long_input_b = this->data.long_source_b = lb;

	/* Pre-process input files */
	if (this->fa_input) {
		ab = (uchar*)pz_malloc((la+1+lb)*sizeof(uchar));
		this->data.long_input_a = this->data.long_source_a = la = fa_copy_cont(a, ab, la); pz_free(a);
		forn(i, la) bc[(uchar)ab[i]]++;
		if (this->fa_strip_n) {
			this->trac_middle = la + !!this->use_separator;
			this->data.long_input_a = la = (this->trac_positions)?fa_strip_n_trac(ab, ab, la, &(this->trac_buf), &(this->trac_size), 0):fa_strip_n(ab, ab, la);
		}
		if (this->use_separator) ab[la++] = 254;

		this->data.long_input_b = this->data.long_source_b = lb = fa_copy_cont(b, ab+la, lb); pz_free(b);
		forsn(i, la, la+lb) bc[(uchar)ab[i]]++;
		if (this->fa_strip_n) {
			this->data.long_input_b = lb = (this->trac_positions)?fa_strip_n_trac(ab+la, ab+la, lb, &(this->trac_buf), &(this->trac_size), la):fa_strip_n(ab+la, ab+la, lb);
		}
		this->data.N_blocks = this->trac_size;

		// la includes separator (if any).
		n = la + lb + !!this->use_terminator;
		this->s = (uchar*)pz_malloc(n*sizeof(uchar));
		s = (uchar*) this->s;
		if (this->use_terminator || this->use_separator) {
			if (la+lb) memcpy(s, ab, la+lb);
			s[n-1] = 255; // Set the terminator
		} else {
			if (n != fa_encode(ab, s, n)) {
				fprintf(stderr, "Error al encodear archivos concatenados.\n");
			}
		}
		pz_free(ab);
		if (this->trac_positions) ppz_kpw_save_trac(this);
	} else {
		n = la + !!this->use_separator + lb + !!this->use_terminator;
		this->s = (uchar*)pz_malloc(n*sizeof(uchar));
		s = (uchar*) this->s;
		if (la) memcpy(s, a, la); pz_free(a);
		if (this->use_separator) s[la] = 254; // Set the middle separator
		if (lb) memcpy(s+la+!!this->use_separator, b, lb); pz_free(b);
		if (this->use_terminator) s[n-1] = 255; // Set the terminator
		forn(i, n) bc[(uchar)s[i]]++;
	}

	this->data.n = n;
	this->data.cnt_A = bc['A'];
	this->data.cnt_C = bc['C'];
	this->data.cnt_G = bc['G'];
	this->data.cnt_T = bc['T'];
	this->data.cnt_N = bc['N'];
	this->update_data = TRUE;
	this->p = (uint*)pz_malloc(n*sizeof(uint)); /* 4N */
	this->r = (uint*)pz_malloc(n*sizeof(uint)); /* 4N */
	this->bwto = (uchar*)pz_malloc(n*sizeof(uchar)); /* 1N */

	this->st = PZ_ST_LOADED;
	return 1;
}

/** Section STATS **/

bool ppz_kpw_load_stats(ppz_status* this) {
	uint64 ld = kpw_sec_load_into(this->kpwf, KPW_ID_STATS, &(this->data), sizeof(this->data));
	if (ld != sizeof(this->data)) this->update_data = 1;
	return ld;
}

void ppz_kpw_save_stats(ppz_status* this) {
	kpw_save_stats(this->kpwf, &this->data);
}

/** Section TRAC **/

void ppz_trac_load(ppz_status* this, const char* fn) {
	uchar *a;
	uint la, lr = 0;
#ifdef GZIP
	char fngz[MAX_FILENAME_GZ];
#endif

	if (!(this->fa_input) || !(this->trac_positions) || !(this->fa_strip_n)) return;

	TRY_LOAD_FILE(fn, a, la)
	if (!a) { ppz_error("Error loading file: '%s'", fn); return; }

	la = fa_copy_cont(a, a, la);
	if (this->trac_middle != 0) lr = this->data.long_input_a + !!this->use_separator;
	lr = fa_strip_n_trac(a, a, la, &(this->trac_buf), &(this->trac_size), lr);
	if (this->trac_middle == 0) {
		this->data.long_input_a = lr;
		this->trac_middle = la + !!this->use_separator;
	}

	pz_free(a);
}

bool ppz_kpw_load_trac(ppz_status* this) {
	uint *trac_buf;

	trac_buf = kpw_load_trac(this->kpwf, &this->trac_size, &this->trac_middle);
	if (!trac_buf) return FALSE;
	if (this->trac_buf) pz_free(this->trac_buf);
	this->trac_buf = trac_buf;

	return TRUE;
}
bool ppz_kpw_save_trac(ppz_status* this) {
	return kpw_save_trac(this->kpwf, this->trac_buf, this->trac_size, this->trac_middle);
}

/** Section BWT **/

int ppz_kpw_load_bwt(ppz_status* this) {
	uchar* bwto;
	bwto = kpw_load_bwt(this->kpwf, &this->data.n, &this->prim);
	if (this->p) pz_free(this->p);
	if (this->r) pz_free(this->r);
	if (this->s) pz_free(this->s);
	if (this->bwto) pz_free(this->bwto);

	this->p = pz_malloc(sizeof(uint) * this->data.n);
	this->r = pz_malloc(sizeof(uint) * this->data.n);
	this->s = pz_malloc(sizeof(uchar) * this->data.n);
	this->bwto = bwto;

	bwt_spr(this->bwto, this->p, this->r, this->s, this->data.n, this->prim);

	this->st = PZ_ST_BWT;
	return this->data.n;
}

bool ppz_kpw_save_bwt(ppz_status* this) {
	return kpw_save_bwt(this->kpwf, this->bwto, this->data.n, this->prim);
}

int ppz_load_bwt(ppz_status* this, const char* fn) {
	ppz_ensure(this->st == PZ_ST_CREATED, 0);
#ifdef GZIP
	char fngz[MAX_FILENAME_GZ];
#endif
	uchar *mem = NULL;
	uint64 *prm;
	uint n;

	TRY_LOAD_FILE(fn, mem, n)
	if (!mem) { ppz_error("Error loading file: '%s'", fn); return 0; }

	// The prim number is located at the end of the file.
	n -= 2*sizeof(uint64);
	prm = (uint64*)(mem + n);

	if ((n < 0) || (*prm > n)) { pz_free(mem); ppz_error("Invalid BWT file: '%s'", fn); return 0; }

	this->prim = prm[0];
	this->data.long_input_a = prm[1];
	this->data.n = n;
	this->p = pz_malloc(sizeof(uint)*n);

	memcpy(this->p, mem, n);
	pz_free(mem);
	this->r = pz_malloc(sizeof(uint)*n);

	bwt_spr(NULL, this->p, this->r, NULL, this->data.n, this->prim);

	this->st = PZ_ST_BWT;
	return n;
}

int ppz_save_bwt(ppz_status* this, const char* fn) {
	ppz_ensure(this->st == PZ_ST_BWT, 0);
	uint64 lval[2], *prm = (uint64*)(((uchar*)this->p) + this->data.n);
	uint n, i;
	// Stores prim at the end of the file.
	n = this->data.n + 2*sizeof(uint64);
	forn(i, 2) lval[i] = prm[i];
	prm[0] = this->prim;
	prm[1] = this->data.long_input_a;
//	DBGcn((uchar*)this->p, n+8);
//	DBGun((uchar*)this->p, n+8);

#ifdef GZIP
	if (this->gz_input) {
		return saveStrGzFile(fn, (uchar*)this->p, n);
	} else
#endif
	{
		return saveStrFile(fn, (uchar*)this->p, n);
	}
	forn(i, 2) prm[i] = lval[i]; // Restores the bufer
}

void ppz_kpw_open(ppz_status* this, const char* fn) {
	if (this->kpwf) kpw_close(this->kpwf);
	this->kpwf = kpw_open(fn);
	if (!this->kpwf) { ppz_error("Error loading .kpw file: %s", fn); return; }
	// Carga ciertas cosas
	if (this->st == PZ_ST_CREATED) {
		ppz_kpw_load_stats(this);
	}
}

typedef void (*bwt_funcptr)(uchar*, uint*, uint*, uchar*, uint, uint*);

void ppz_bwt_gen(ppz_status* this, bwt_funcptr bwtf) {
	tiempo t1, t2;
	uchar *s;
	ppz_ensure(this->st == PZ_ST_LOADED, );

	if (this->kpwf && !KPW_IT_END(kpw_sec_find(this->kpwf, KPW_ID_BWT))) {
		// Carga la BWT de disco
		ppz_kpw_load_bwt(this);
	} else {
		getTickTime(&t1);
		// Llamar a bwt con p, r y n. p contiene la entrada de tamaño n al comienzo.
		bwtf(this->bwto, this->p, this->r, this->s, this->data.n, &(this->prim));
		getTickTime(&t2);
		this->data.time_bwt = (uint)(getTimeDiff(t1, t2));

		s = this->s?this->s:(uchar*)this->p;
		if (this->fa_input && !this->use_terminator) {
			fa_decode(s, s, this->data.n);
		}
		ppz_kpw_save_bwt(this);
		this->update_data = TRUE;
	}

	this->st = PZ_ST_BWT;
	return;
}

void ppz_bwt(ppz_status* this){
	ppz_bwt_gen(this, &bwt);
}

void ppz_obwt(ppz_status* this){
	ppz_bwt_gen(this, &obwt);
}

/* LCP */

void ppz_lcp(ppz_status* this, uint save) {
	this->st = PZ_ST_LOADED;
	if (this->kpwf && !KPW_IT_END(kpw_sec_find(this->kpwf, KPW_ID_LCP))) {
		/* Carga la LCP de disco */
		uint64 lcpn = 0;
		this->h = kpw_load_lcp(this->kpwf, &lcpn);
		if (this->h == NULL) {
			ppz_error("%s", "Couldn't load LCP section.");
		}
		if (lcpn != sizeof(uint) * this->data.n) {
			ppz_error("%s", "Warning, size of LCP and size of input differ.");
		}
	} else {
		/* Genera (o carga) la BWT y genera la LCP */
		ppz_bwt(this);
		lcp(this->data.n, this->s, this->r, this->p);
		this->h = this->r;
		this->h[this->data.n - 1] = 0;
		pz_free(this->s); this->s = NULL;
		pz_free(this->p); this->p = NULL;
		pz_free(this->bwto); this->bwto = NULL;
		this->r = NULL;
		if (save) {
			if (!kpw_save_lcp(this->kpwf, this->h, this->data.n)) {
				ppz_error("%s", "Couldn't save LCP section.");
			}
		}
	}
	this->st = PZ_ST_LCP;
	return;
}

void ppz_klcp(ppz_status* this, uint save) {
	/* FIXME: Verificar prerequisitos y postcondiciones */
	uchar *hset = NULL;
	uint* h = NULL;
	tiempo t1, t2;
	getTickTime(&t1);

	/* Reusa LCP si ya existe */
	if (this->h) {
		h = this->h;
	} else {
		/* Intenta cargar el lcpset */
		hset = kpw_load_lcpset(this->kpwf, NULL);
		if (hset) {
			save = 0; /* Don't save if already load */
		} else {
			/* y bueh, se calcula */
			ppz_lcp(this, 0);
			h = this->h;
		}
	}

	long double res[KLCP_FUNCS];

	if (!hset) {
		hset = (uchar*)malloc((this->data.n -1 +7)/8);
		lcp_arrtoset_sort(h, hset, this->data.n);
	}
	klcpset_f_arr(this->data.n, hset, res);

	getTickTime(&t2);
	uint tms = (uint)(getTimeDiff(t1, t2));

	uint j;
	printf("%d\t%s\t%s\t%d", SVNREVISION, this->data.filename_a, this->data.filename_b, tms);
	forn(j, KLCP_FUNCS) printf("\t%.6lf", (double)res[j]);
	printf("\n");
	
	this->data.kinv_lcp = res[4]; // 1/(x+1.0)
	this->update_data = 1;

	if (save) {
		kpw_save_lcpset(this->kpwf, hset, this->data.n);
	}

	if (hset) pz_free(hset);

	if (h && this->h != h) pz_free(h);
}

/* Greedy para long_pat */
typedef struct str_data_greedy {
	uint64 cost;
	uint lim;
} data_greedy;
void dg_init(data_greedy* dg) {
	dg->cost = 0;
	dg->lim = 0;
}

void greedy_step(uint i, uint v, uint dl, data_greedy* dg, const uint chcost) {
	uint nlim;
	uint64 ncost;
	if (v != 0) {
		nlim = i + v;
		ncost = bneed(v) + bneed(dl);
		if ((nlim > dg->lim) && (ncost < (uint64)(nlim - dg->lim) * chcost)) {
			dg->lim = nlim;
			dg->cost += ncost;
//			if (chcost==8) fprintf(stderr, "at %u, (%u, %u)\tcost=%llu\n", i, v, dl, dg->cost);
		}
	}
	if (i >= dg->lim) {
//		fprintf(stderr, "at %u, char %c\n", i, dg->s[i]);
		dg->lim = i+1;
		dg->cost += chcost;
	}
}

void greedy_step_dl0(uint i, uint v, uint dl, data_greedy* dg, const uint chcost) {
	uint nlim;
	uint64 ncost;
	if (v != 0) {
		nlim = i + v;
		ncost = bneed(v) + 0;
		if ((nlim > dg->lim) && (ncost < (uint64)(nlim - dg->lim) * chcost)) {
			dg->lim = nlim;
			dg->cost += ncost;
//			fprintf(stderr, "at %u, (%u, %u)\tcost=%llu\n", i, v, dl, dg->cost);
		}
	}
	if (i >= dg->lim) {
//		fprintf(stderr, "at %u, char ?\n", i);
		dg->lim = i+1;
		dg->cost += chcost;
	}
}

void _greedy_step(uint i, uint v, uint dl, data_greedy* dg) {
	greedy_step(i, v, dl, dg+0, 8);
	greedy_step(i, v, dl, dg+1, 3);
	greedy_step_dl0(i, v, dl, dg+2, 3);
}

void ppz_longpat(ppz_status* this) {
	tiempo t1, t2;
	uint *h, *p, *r, n, i;
	bittree *bwtt;
	uint bc[256];
	ppz_ensure(this->st == PZ_ST_BWT, );
	getTickTime(&t1);

	p = this->p;
	r = this->r;
	n = this->data.n;

	bwt_build_bc(this->bwto, n, bc);
	bwtt = longpat_bwttree(this->bwto, n); // 0,25 *N
	pz_free(this->bwto); this->bwto = NULL;// -1 *N

	h = r; this->r = NULL; // robo r, 4*N
//	memcpy(h, r, n*sizeof(uint));
	lcp(n, this->s, h, p);

	if (this->s) { pz_free(this->s); this->s = NULL; } // -1 * N

	data_greedy dg[3];
	forn(i, 3) dg_init(dg+i);

	longpat(NULL, n, NULL, h, p, bwtt, (longpat_callback*)_greedy_step, dg);

	pz_free(h);
	bittree_free(bwtt, n+1);

	this->data.lpat_size = dg[0].cost;
	this->data.lpat3_size = dg[1].cost;
	this->data.lpat3dl0_size = dg[2].cost;
	this->update_data = TRUE;

	this->r = pz_malloc(n*sizeof(uint)); // 4 * N
	this->s = pz_malloc(n*sizeof(uchar)); // 1 * N
	this->bwto = pz_malloc(n*sizeof(uchar)); // 1 * N
	bwt_rsrc_pbc(this->bwto, this->p, this->r, this->s, n, bc);

	getTickTime(&t2);
	this->data.time_longp = (uint)(getTimeDiff(t1, t2));
}

/* MRS / findpat */
typedef struct {
	uint cnt, n, encr;
	uint ll, lc;
	uint64 loff;
	kpw_file* kpwf;
	obitstream obs;
} mrs_data;

void mrsdata_init(mrs_data* this, kpw_file* kpwf, uint n) {
	this->cnt = 0;
	this->ll = this->lc = 0;
	this->kpwf = kpwf;
	this->n = n;
	this->encr = bneed(n-1);
}

void mrsdata_len_open(mrs_data* this, uint l) {
	this->loff = ftell(this->kpwf->f);
	kpw_sec_patt_subsec_open(this->kpwf);
	obs_create(&(this->obs), this->kpwf->f);
}

bool mrsdata_len_close(mrs_data* this) {
	if (!this->lc) return 0;
//	printf("%u\t%u\n", this->ll, this->lc);
	obs_destroy(&(this->obs));
	return kpw_sec_patt_subsec_close(this->kpwf, this->loff, this->ll, this->lc);
}

static void mrs_callback(uint l, uint i, uint n, mrs_data* dat) {
	dat->cnt++;
	if (l != dat->ll) {
		if (dat->lc) mrsdata_len_close(dat);
		dat->lc = 0;
		dat->ll = l;
		mrsdata_len_open(dat, l);
	}
	/* Stores the index of r[] in dat->encr fixed bits (same bits as n) */
	obs_write_uint(&(dat->obs), dat->encr, i);
//	DBGu(i);
	/* Stores the number of occurrences in 2-bit-per-bit encoding */
	obs_write_bbpb(&(dat->obs), n);
//	DBGu(n);
	dat->lc++;
}


typedef void (*mrs_funcptr)(uchar *, uint, uint *, uint *, uint *, uint, output_callback, void *);

void ppz_mrs_gen(ppz_status* this, uint save, uint ml, mrs_funcptr mrsf) {
	tiempo t1, t2;
	uint *h, *p, *r, n;
	mrs_data dt;
//	ppz_ensure(this->st == PZ_ST_BWT, );
	getTickTime(&t1);

	if (this->bwto) { pz_free(this->bwto); this->bwto = 0; }
	n=this->data.n;
	p=this->p;
	r=this->r;

	// LCP
//	fprintf(stderr,"start lcp\n");
	h = (uint*)pz_malloc(n*sizeof(uint)); // 4*N
	memcpy(h, r, n*sizeof(uint));
	lcp(n, this->s, h, p);

	// MRS
//	fprintf(stderr,"start mrs\n");
	mrsdata_init(&dt, this->kpwf, n);

	kpw_idx_it it = kpw_sec_find(this->kpwf, KPW_ID_PATT);
	if (!KPW_IT_END(it)) {
		fprintf(stderr, "Removing old PATT secction\n");
		kpw_sec_delete(this->kpwf, it);
	}
	kpw_sec_new(this->kpwf, KPW_ID_PATT);

	mrsf(this->s, n, r, h, p, ml, (output_callback*)mrs_callback, &dt);

	pz_free(h);

//	if (dt.lc) printf("%u\t%u\n", dt.ll, dt.lc);
	if (dt.lc) mrsdata_len_close(&dt);

	kpw_sec_write_upd(this->kpwf);
	kpw_sec_close(this->kpwf);

//	DBGu(dt.cnt);

	getTickTime(&t2);
	this->data.time_mrs = (uint)(getTimeDiff(t1, t2));
	this->update_data = 1;
}

void ppz_mrs(ppz_status* this, uint save, uint ml) {
	ppz_mrs_gen(this, save, ml, &mrs);
}

void ppz_mrs_nolcp(ppz_status* this, uint save, uint ml) {
	ppz_mrs_gen(this, save, ml, &mrs_nolcp);
}

/** MMRS **/

void ppz_mmrs(ppz_status* this, uint save, uint ml) {
	tiempo t1, t2;
	uint *h, *p, *r, n;
//	ppz_ensure(this->st == PZ_ST_BWT, );
	getTickTime(&t1);

	if (this->bwto) { pz_free(this->bwto); this->bwto = 0; }


	n=this->data.n;
	p=this->p;
	r=this->r;

	/* Calculate the LCP */
	h = (uint *)pz_malloc(n * sizeof(uint));
	memcpy(h, r, n * sizeof(uint));
	lcp(n, this->s, h, p);

	output_readable_data ord;
	ord.fp = stdout;
	ord.r = r;
	ord.s = this->s;
	ord.trac_buf = this->trac_buf;
	ord.trac_size = this->trac_size;
	ord.trac_middle = this->trac_middle;


	/* FIXME: El output de MMRS va siempre a pantalla */
	/*
	kpw_idx_it it = kpw_sec_find(this->kpwf, KPW_ID_PATT);
	if (!KPW_IT_END(it)) {
		fprintf(stderr, "Removing old PATT secction\n");
		kpw_sec_delete(this->kpwf, it);
	}
	kpw_sec_new(this->kpwf, KPW_ID_PATT);
	*/

	mmrs(this->s, n, r, h, ml, output_readable_trac, &ord);

	pz_free(h);

	/* FIXME: El output de MMRS va siempre a pantalla */
	/*
	if (dt.lc) mrsdata_len_close(&dt);

	kpw_sec_write_upd(this->kpwf);
	kpw_sec_close(this->kpwf);
	*/

	getTickTime(&t2);
	this->data.time_mmrs = (uint)(getTimeDiff(t1, t2));
	this->update_data = 1;
}

void ppz_mtf(ppz_status* this) {
	tiempo t1, t2;
	ppz_ensure(this->st < PZ_ST_MTF && this->st >= PZ_ST_LOADED, );

	getTickTime(&t1);
	this->data.zero_mtf = mtf_log(this->bwto, this->bwto, this->data.n);
	getTickTime(&t2);
	this->data.time_mtf = (uint)(getTimeDiff(t1, t2));
	this->update_data = 1;

	this->st = PZ_ST_MTF;
	return;
}

void ppz_mtf_ests(ppz_status* this) {
	ppz_ensure(this->st == PZ_ST_MTF, );

	// nzz_est
	uchar *p = (uchar*)(this->p);
	uint n = this->data.n, nzz = 0;
	if (n--) while (n--) nzz += (*(p++)) && !(*p);
	this->data.nzz_est = nzz;

	// mtf_entropy
	this->data.mtf_entropy = ((uint64)ceil(entropy_char(this->bwto, this->data.n)*this->data.n)+(uint64)7)/(uint64)8;
}

void ppz_lzw(ppz_status* this) {
	tiempo t1, t2;
	void* p;
	ppz_ensure(this->st < PZ_ST_LZW && this->st >= PZ_ST_LOADED, );
	getTickTime(&t1);
	/* Libera la RAM no usada */
	if (this->r) { pz_free(this->r); this->r = 0; }
	p = this->p;
	this->p = pz_malloc(this->data.n * sizeof(uchar));
	memcpy(this->p, p, this->data.n);
	pz_free(p);
	this->lzw_cds = pz_malloc(this->data.n * sizeof(ushort)); /* A lo sumo LZW escribe n códigos */
	/* Llama a LZW */
	this->data.lzw_codes = this->data.nlzw = lzw((uchar*)(this->p), this->lzw_cds, this->data.n);
	getTickTime(&t2);
	this->data.time_lzw = (uint)(getTimeDiff(t1, t2));
	this->st = PZ_ST_LZW;
	return;
}

void ppz_est_size(ppz_status* this) {
	this->data.est_size = arit_enc_est(this->lzw_cds, this->data.nlzw);
}

void ppz_huffman_len(ppz_status* this) {
	tiempo t1, t2;
	ppz_ensure(this->st == PZ_ST_LZW, );
	getTickTime(&t1);

	this->data.huf_len = (huffman_compress_len(this->lzw_cds, this->data.nlzw, LZW_DICTSIZE, &(this->data.huf_tbl_bit))+7)/8;

	getTickTime(&t2);
	this->data.time_huf = (uint)(getTimeDiff(t1, t2));
}

uint ppz_trac_convert(ppz_status* this, uint x) {
	if (!this->trac_buf) return x;
	return trac_convert_pos_virtual_to_real(x, this->trac_buf, this->trac_size);
}

void ppz_write_out(ppz_status* this, const char* fn) {

}

#define MAX_DATA_OUTPUT 4096
void ppz_write_stats(ppz_status* this, const char* fn) {
	char buf[MAX_DATA_OUTPUT];
	uint zbuf = MAX_DATA_OUTPUT-1, ibuf, cont, mcont;

	#define buf_add(...) { ibuf += snprintf(buf+ibuf, zbuf-ibuf, __VA_ARGS__); if (ibuf > zbuf) ibuf = zbuf; }
	#define add_col(fmt, fld) { \
		if (cont) { buf_add(fmt"\t", this->data.fld); } else { buf_add(#fld"\t") } }

	FILE* f = fopen(fn, "r");
	mcont = 1;
	if (!f) {
		f = fopen(fn, "w");
		if (!f) { fprintf(stderr, "error creating %s.\n", fn); return; }
		mcont = 0;
	} else {
		fclose(f);
		f = fopen(fn, "a");
		if (!f) return;
	}

	forsn(cont, mcont, 2) {
		ibuf = 0;
		add_col("%u", rev)
		add_col("%s", filename_a)
		add_col("%s", filename_b)
		add_col("%u", long_source_a)
		add_col("%u", long_source_b)
		add_col("%u", long_input_a)
		add_col("%u", long_input_b)
		add_col("%u", n)
		add_col("%u", zero_mtf)
		add_col("%u", nzz_est)
		add_col("%u", mtf_entropy)
		add_col("%u", lzw_codes)
		add_col("%u", huf_len)
		add_col("%u", huf_tbl_bit)
		add_col("%u", long_output)
		add_col("%u", est_size)
		add_col("%llu", lpat_size)
		add_col("%llu", lpat3_size)
		add_col("%llu", lpat3dl0_size)
		add_col("%.6lf", kinv_lcp)
		add_col("%u", time_bwt)
		add_col("%u", time_mtf)
		add_col("%u", time_lzw)
		add_col("%u", time_huf)
		add_col("%u", time_longp)
		add_col("%u", time_mrs)
		add_col("%u", time_mmrs)
		add_col("%u", time_tot)
		add_col("%s", hostname)

		buf[ibuf-1] = '\n';
		if (!fwrite(buf, 1, ibuf, f)) perror("ERROR writing logfile");
	}
	fclose(f);
}

/* Builds string: [dirname(outn)]/[pfx]-[basename(in1)]-[basename(in2)][ext] */
char* std_name(const char* outn, const char* pfx, const char* in1, const char* in2, const char* ext) {
	int l1 = strlen(in1);
	int l2 = in2?strlen(in2):0;
	int lo = outn?strlen(outn):0;
	int lp = pfx?strlen(pfx):0;
	int le = ext?strlen(ext):0;
	char* out;
	while(lo && outn[lo-1] != '/') lo--;
	out = (char*)pz_malloc((lo+1+lp+1+l1+1+l2+1+le+1)*sizeof(char));

	if (lo) memcpy(out, outn, lo);

	if (pfx) {
		strcpy(out+lo, pfx); lo += lp;
		out[lo++] = '-';
	}

	while(l1 && in1[l1-1] != '/') l1--;
	strcpy(out+lo, in1+l1); lo = strlen(out);

	if (in2) {
		out[lo++] = '-';
		while(l2 && in2[l2-1] != '/') l2--;
		strcpy(out+lo, in2+l2); lo = strlen(out);
	}

	if (ext) {
		out[lo++] = '.';
		strcpy(out+lo, ext); lo += le;
	}
	return out;
}

/*** Functions to deal with kpw files by name ***/

/* Creates path/file1-file2.kpw with DATA, TRAC and BWT sections. */
bool _kpwfile_create(ppz_status *pz, char *name, char *path, char *file1, char *file2) {
	char *out = std_name(path, NULL, file1, file2, "kpw");
	strncpy(name, out, PPZ_MAX_KPW_FILENAME);
	pz_free(out);
	ppz_kpw_open(pz, name);

	tiempo t1, t2;
	getTickTime(&t1);
	ppz_load_file2(pz, file1, file2);

	if (pz->st == PZ_ST_LOADED) {
		ppz_bwt(pz);
		/*ppz_mtf(pz); ppz_mtf_ests(pz);*/
		/*if (pz->bwto) { pz_free(pz->bwto); pz->bwto = NULL; }*/
		getTickTime(&t2);
		pz->data.time_tot = (uint)(getTimeDiff(t1, t2)); pz->update_data = TRUE;
	} else {
		fprintf(stderr, "%s", "WARNING: couldn't load files.\n");
		return FALSE;
	}
	return TRUE;
}

int ppz_kpwfile_find(ppz_status *pz, char *kpwfn, char *path, char *file1, char *file2) {
	char *s;
	s = std_name(path, NULL, file1, file2, "kpw");
	if (fileexists(s)) {
		fprintf(stderr, "Found <%s>\n", s);
		strncpy(kpwfn, s, PPZ_MAX_KPW_FILENAME);
		pz_free(s);
		ppz_kpw_open(pz, kpwfn);
		return PPZ_LEFT;
	}
	pz_free(s);
	s = std_name(path, NULL, file2, file1, "kpw");
	if (fileexists(s)) {
		fprintf(stderr, "Found <%s>\n", s);
		strncpy(kpwfn, s, PPZ_MAX_KPW_FILENAME);
		pz_free(s);
		ppz_kpw_open(pz, kpwfn);
		return PPZ_RIGHT;
	}
	pz_free(s);
	fprintf(stderr, "Creating KPW file for <%s>, <%s>\n", file1, file2);
	if (_kpwfile_create(pz, kpwfn, path, file1, file2))
		return PPZ_NEW;
	else
		return FALSE;
}

int ppz_kpwfile_load(ppz_status *pz, char *path, char *file1, char *file2) {
	char name[PPZ_MAX_KPW_FILENAME];
	int loaded = ppz_kpwfile_find(pz, name, path, file1, file2);
	if (!loaded) {
		fprintf(stderr, "Cannot find KPW file for <%s>, <%s>\n", file1, file2);
		return FALSE;
	}
	if (loaded != PPZ_NEW) {
		if (!ppz_kpw_load_stats(pz)) {
			fprintf(stderr, "Cannot load DATA section for <%s>, <%s>\n", file1, file2);
			return FALSE;
		}
		if (!ppz_kpw_load_trac(pz)) {
			fprintf(stderr, "Cannot load TRAC section for <%s>, <%s>\n", file1, file2);
			pz->trac_buf = 0;
			pz->trac_middle = pz->trac_size = 0;
//			return FALSE;
		}
		if (!ppz_kpw_load_bwt(pz)) {
			fprintf(stderr, "Cannot load BWT section for <%s>, <%s>\n", file1, file2);
			return FALSE;
		}
	}
	return loaded;
}

