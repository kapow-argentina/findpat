#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libp2zip.h"
#include "bwt.h"
#include "lcp.h"
#include "mrs.h"
#include "mmrs.h"
#include "common.h"

#include "macros.h"

/* SAVE_TRIPLES guarda los patrones en un archivo 'triples.dat', for debug */
/* # define SAVE_TRIPLES */

#define BWT 0
#define OBWT 1
#define MRS 0
#define MMRS 1

/* set<int> */
#define SET_INT_SZ 1024
typedef struct str_set_int {
	int cnt[SET_INT_SZ];
	int cap, l, tot;
	int* vec;
} set_int;
void set_int_clear(set_int* this) {
	memset(this->cnt, 0, sizeof(this->cnt));
	this->l = 0;
	this->tot = 0;
}
void set_int_create(set_int* this) {
	this->cap = 128;
	this->vec = pz_malloc(this->cap*2*sizeof(int));
	set_int_clear(this);
}
void set_int_destroy(set_int* this) {
	pz_free(this->vec);
}
void set_int_resize(set_int* this, int ncap) {
	int *p = pz_malloc(ncap*2*sizeof(int));
	memcpy(p, this->vec, this->l*2*sizeof(int));
	pz_free(this->vec);
	this->vec = p;
	this->cap = ncap;
}
void set_int_add(set_int* this, int x) {
	int i;
	this->tot++;
	if (x < SET_INT_SZ) { this->cnt[x]++; return; }
	forn(i, this->l) if (this->vec[2*i] == x) { this->vec[2*i+1]++; return; }
	if (this->l == this->cap) set_int_resize(this, 2*this->cap);
	this->vec[2*this->l] = x;
	this->vec[2*this->l+1] = 1;
	this->l++;
}

/* General options */
int files_gz=1;
int files_fa=1;
int strip_n=1;
char* save_bwt = NULL;

#define ONLY_LEFT 1
#define ONLY_RIGHT 2
#define ONLY_BOTH 3
#define ONLY_ALL 0
struct dto_findpat_struct {
	/* general options */
	uint* r;
	uchar* s;
	int trac_pos, full_pos, middle, only;

	/* full mode (-p) */
	int mode_full;
	FILE* f_p;
	char* fn_p;
	int ml_p;

	/* statistics mode (-S) */
	int mode_stats;
	FILE* f_S;
	char* fn_S;
	int ml_S;

	/* small-statistics mode (-s) */
	int mode_small;
	FILE* f_s;
	char* fn_s;
	set_int si;
	int llen, ml_s;
};
typedef struct dto_findpat_struct dto_findpat;

void dfp_create(dto_findpat* this) {
	this->mode_full = 0;
	this->mode_stats = 0;
	this->mode_small = 0;
	this->only = ONLY_ALL;
	this->trac_pos = 1;
	this->full_pos = 1;

	this->r = NULL;
	this->s = NULL;
	this->fn_p = this->fn_S = this->fn_s = NULL;
	this->llen = -1;
	this->middle = 0;
}

void dfp_destroy(dto_findpat* this) {
	if (this->fn_p) { pz_free(this->fn_p); this->fn_p = 0; }
	if (this->fn_S) { pz_free(this->fn_S); this->fn_S = 0; }
	if (this->fn_s) { pz_free(this->fn_s); this->fn_s = 0; }
	if (this->mode_small) set_int_destroy(&(this->si));
}

#define DFP_OK 1
#define DFP_NOT_FULL 2
#define DFP_NOT_STATS 3
#define DFP_NOT_SMALL 4
int dfp_open(dto_findpat* this) {
	if (this->mode_full) {
		this->f_p = fopen(this->fn_p, "wt");
		if (!this->f_p) return DFP_NOT_FULL;
	}
	if (this->mode_stats) {
		this->f_S = fopen(this->fn_S, "wt");
		if (!this->f_S) {
			if (this->mode_full) fclose(this->f_p);
			return DFP_NOT_STATS;
		}
	}
	if (this->mode_small) {
		this->f_s = fopen(this->fn_s, "wt");
		if (!this->f_s) {
			if (this->mode_full) fclose(this->f_p);
			if (this->mode_stats) fclose(this->f_S);
			return DFP_NOT_SMALL;
		}
	}
	if (this->mode_small) {
		fprintf(this->f_s, "len\treps\t#pats\n");
		set_int_create(&(this->si));
	}
	if (this->mode_stats) {
		fprintf(this->f_S, "len\treps\t#<\t#>\n");
	}
	return DFP_OK;
}

void dfp_print_pats(FILE* f, set_int* si, int len);

void dfp_close(dto_findpat* this) {
	if (this->mode_full) fclose(this->f_p);
	if (this->mode_stats) fclose(this->f_S);
	if (this->mode_small) {
		dfp_print_pats(this->f_s, &(this->si), this->llen);
		fclose(this->f_s);
	}
}

ppz_status pz;

void dfp_print_pats(FILE* f, set_int* si, int len) {
	int i;
	forn(i, SET_INT_SZ) if (si->cnt[i]) fprintf(f, "%u\t%u\t%u\n", len, i, si->cnt[i]);
	forn(i, si->l) fprintf(f, "%u\t%u\t%u\n", len, si->vec[2*i], si->vec[2*i+1]);
	set_int_clear(si);
}

#ifdef SAVE_TRIPLES
FILE* f_triples;
#endif

void dfp_output(uint l, uint i, uint n, void* vdfp) {
	uint j;
	dto_findpat* dfp = (dto_findpat*)vdfp;
	int cn[2]; cn[0] = cn[1] = 0;
	forn(j, n) cn[dfp->r[i+j] >= dfp->middle]++;
	if (dfp->only == ONLY_BOTH && (!cn[0] || !cn[1])) return;
	if (dfp->only == ONLY_LEFT && cn[1]) return;
	if (dfp->only == ONLY_RIGHT && cn[0]) return;

	if (dfp->mode_stats) { // Print statistics mode
		fprintf(dfp->f_S,"%u\t%u\t%u\t%u\n", l, n, cn[0], cn[1]);
	}

	if (dfp->mode_small) { // Print small statistics mode
		if (l != dfp->llen) {
			dfp_print_pats(dfp->f_s, &(dfp->si), dfp->llen);
			dfp->llen = l;
		}
		set_int_add(&dfp->si, n);
	}

	if (dfp->mode_full) { // Print expanded (full mode)
		forn(j,l) fprintf(dfp->f_p, "%c", dfp->s[dfp->r[i]+j]);
		fprintf(dfp->f_p," #%u (%u)\n", n, l);

		if (dfp->full_pos) {
			fprintf(dfp->f_p," ");
			forn(j,n) fprintf(dfp->f_p, " %c%d", (dfp->r[i+j]<dfp->middle?'<':'>'),
			 ((dfp->trac_pos)?
			  (dfp->r[i+j]<dfp->middle?ppz_trac_convert(&pz, dfp->r[i+j]):ppz_trac_convert(&pz, dfp->r[i+j])-pz.trac_middle):
			  (dfp->r[i+j]<dfp->middle?dfp->r[i+j]:dfp->r[i+j]-dfp->middle))
			);
			fprintf(dfp->f_p,"\n");
		}
	}
#ifdef SAVE_TRIPLES
/*
	uint data[3] = {l, i, n};
	fwrite(data, 3, sizeof(uint), f_triples);
*/
	if (l >= 1) fprintf(f_triples, "%u\t%d\n", l, n);
#endif
}

const char* prefixes[4][3] = {
	{"full",  "stat",  "smal"},
	{"fullL", "statL", "smalL"},
	{"fullR", "statR", "smalR"},
	{"full2", "stat2", "smal2"} };

void findpat(const char* in1, const char* in2, const char* outn, uint ml, dto_findpat* dfp, int flags) {
	/*Read file and get BWT*/
	uint n;
	uint *p, *r, *h;
	char* bwtf = NULL;
	const char* prefix[3];
	int dfp_open_err;

	memcpy(prefix, prefixes[dfp->only], sizeof(prefix));

	ppz_create(&pz);
	pz.fa_input = files_fa;
	pz.gz_input = files_gz;
	pz.fa_strip_n = strip_n;

	pz.use_terminator = 1;
	pz.use_separator = 1;
	pz.trac_positions = dfp->trac_pos;

	/* Try to load the precalculated BWT */
	bwtf = std_name(outn, NULL, in1, in2, "bwt");
	if (save_bwt == NULL) save_bwt = bwtf;
	if (ppz_load_bwt(&pz, save_bwt)) {
		/* BWT loaded from file */
		ppz_trac_load(&pz, in1);
		ppz_trac_load(&pz, in2);
	} else {
		ppz_load_file2(&pz, in1, in2);
		if (flags & 1) ppz_obwt(&pz);
		else  ppz_bwt(&pz);
		ppz_write_stats(&pz, "findpat.txt");
//		ppz_save_bwt(&pz, save_bwt);
	}
	pz_free(bwtf);

	srand(31416);

	n=pz.data.n;
	p=pz.p;
	r=pz.r;
	// Obtiene src.
	uchar *src = pz.s; // (uchar*)pz_malloc(n * sizeof(uchar));
//	memcpy(src, ((uchar*)(p+n))-n, n);
	/* Computes p[] and saves r[]*/
//	forn(i, n) p[r[i]] = i;

//	DBGu(n); DBGun(p, n); DBGun(r, n); DBGcn(src, n);
//	DBGun(pz.trac_buf, pz.trac_size*2);

	h = (uint*)pz_malloc(n*sizeof(uint));
	dfp->middle = pz.data.long_input_a;

	fprintf(stderr,"start lcp\n");
	memcpy(h, r, n*sizeof(uint));
	lcp(n, src, h, p);

	fprintf(stderr,"start mrs\n");
	dfp->r = r;
	dfp->s = src;

	dfp->fn_p = std_name(outn, prefix[0], in1, in2, "txt");
	dfp->fn_S = std_name(outn, prefix[1], in1, in2, "txt");
	dfp->fn_s = std_name(outn, prefix[2], in1, in2, "txt");

	dfp_open_err = dfp_open(dfp);
	if (dfp_open_err != DFP_OK) {
		if (dfp_open_err == DFP_NOT_FULL) {
			fprintf(stderr, "Could not open output file %s.\n", dfp->fn_p);
		} else if (dfp_open_err == DFP_NOT_STATS) {
			fprintf(stderr, "Could not open output file %s.\n", dfp->fn_S);
		} else if (dfp_open_err == DFP_NOT_SMALL) {
			fprintf(stderr, "Could not open output file %s.\n", dfp->fn_s);
		} else {
			fprintf(stderr, "Undetermined error when opening output files.\n");
		}
		return;
	}

	if (flags & 2) mmrs(src, n, r, h, ml, dfp_output, dfp);
	else mrs(src, n, r, h, p, ml, dfp_output, dfp);

	dfp_close(dfp);

	pz_free(h);
	//pz_free(src);
	ppz_destroy(&pz);
}


int main(int argc, char* argv[]) {
	fprintf(stderr, "findpat - rev%d\n", SVNREVISION);

	char* files[4];
	int nf = 0, i, bwtv = BWT, mrsv = MRS;
	uint ml = -1;
	dto_findpat dfp;
	dfp_create(&dfp);
	int flag2 = 0, flagl = 0, flagr = 0;

#ifdef SAVE_TRIPLES
	f_triples = fopen("triples.dat", "wt");
#endif

	forsn(i, 1, argc) {
		if (0);
		else cmdline_var(i, "z", files_gz)
		else cmdline_var(i, "f", files_fa)
		else cmdline_var(i, "r", dfp.trac_pos)
		else cmdline_var(i, "2", flag2)
		else cmdline_var(i, "L", flagl)
		else cmdline_var(i, "R", flagr)
		else cmdline_var(i, "S", dfp.mode_stats)
		else cmdline_var(i, "s", dfp.mode_small)
		else cmdline_var(i, "p", dfp.mode_full)
		else cmdline_var(i, "o", dfp.full_pos)
		else cmdline_opt_1(i, "--obwt") { bwtv = OBWT; }
		else cmdline_opt_1(i, "--mmrs") { mrsv = MMRS; }
		else cmdline_opt_2(i, "--bwt") { save_bwt = argv[i]; }
		else cmdline_opt_2(i, "--no-strip-n") { strip_n = 0; }
		else if (cmdline_is_opt(i)) {
				fprintf(stderr, "%s: Unknown option: %s\n", argv[0], argv[i]);
				return 1;
		}
		else { // Default parameter
			if (nf == 4) {
				fprintf(stderr, "%s: Too many parameters: %s\n", argv[0], argv[i]);
				return 1;
			}
			files[nf++] = argv[i];
		}
	}
	if (nf == 4) {
		ml = atoi(files[3]);
	}
	if (nf != 4 || ml <= 0 || (flag2 + flagr + flagl > 1)) {
		fprintf(stderr, "Use: %s [options] <input1> <input2> <output> <min_length>\n", argv[0]);
		fprintf(stderr, "Options: (-/+ before the option activates/deactivates the option)\n"
			"  f    Treats the input as FASTA files when possible (default is on).\n"
			"  z    Treats the input as a compressed gzip file. If it's not compressed ignores this flag.\n"
			"  L | R | 2    Print only patterns in left/right/both file(s) (default is print all). Only use one of these.\n"
			"  o    Print position of each ocurrence of each pattern in full mode (default is on).\n"
			"  r    Show real positions of patterns long lists of Ns (default is on)\n"
			"\n"
			" Output options:\n"
			"  S    Print in statistics mode (one line for each pattern with len and count).\n"
			"  s    Print in small-statistics mode (one line for each pair len/count indicating ocurrences).\n"
			"  p    Print in full mode (patterns and positions).\n"
			"\n"
			" Other options:\n"
			"  --bwt <file.bwt>   Uses <file.bwt> as precalculated BWT.\n"
			"  --obwt	Uses the optimized bwt algorithm.\n"
			"  --mmrs	Only finds the maximal maximal repeated substrings\n"
		);
		return 1;
	}
	if (flag2) dfp.only = ONLY_BOTH;
	else if (flagl) dfp.only = ONLY_LEFT;
	else if (flagr) dfp.only = ONLY_RIGHT;
	else dfp.only = ONLY_ALL;
	if (!dfp.mode_full && !dfp.mode_small && !dfp.mode_stats) dfp.mode_full = 1; // If none set, set full.

	dfp.ml_p = dfp.ml_S = dfp.ml_s = ml;

	findpat(files[0], files[1], files[2], ml, &dfp, bwtv + (mrsv << 1) );

	dfp_destroy(&dfp);

#ifdef SAVE_TRIPLES
	fclose(f_triples);
#endif

	return 0;
}
