#include "kpw.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libp2zip.h"
#include "kpw_check.h"
#include "kpwapi.h"
#include "bitstream.h"
#include "enc.h"
#include "macros.h"
#include "kpwapi.h"

#include "output_callbacks.h"

#define ONLY_LEFT 1
#define ONLY_RIGHT 2
#define ONLY_BOTH 3
#define ONLY_ALL 0

struct options {
	int only;
	uint ml;
	int full_pos;
	int real_pos;
};

char* parse_path(char* path) {
	char *p = path;
	while (*p && *p != ':') p++;
	if (!*p) return path;
	*p = '\0';
	return p+1;
}

kpw_idx_it sec_from_str(kpw_file* kpw, const char* sec) {
	/* Try: type based */
	uint i = 0;
	kpw_idx_it it;
	forn(i, KPW_IDS) {
		if (!strcmp(KPW_STRID(i), sec) && !KPW_IT_END(it = kpw_sec_find(kpw, i))) {
			return it;
		}
	}
	/* Try: number based */
	it = kpw_idx_begin(kpw);
	uint x = -1;
	sscanf(sec, "%u", &x);
	kpw_idx_add(&it, x);
	return it;
}

void fd_copy(FILE* dst, FILE* src, uint64 len) {
	#define FDC_BUF_SIZE 65536
	void* buf = pz_malloc(FDC_BUF_SIZE);
	while (len) {
		uint rd = FDC_BUF_SIZE;
		if (len < (uint64)rd) rd = len;
		rd = fread(buf, 1, rd, src);
		rd = fwrite(buf, 1, rd, dst);
		len -= rd;
		if (!rd) break;
	}
	pz_free(buf);
//	return !len;
}

/* cmds helpers */

#define kpw_open_or_die(kpw, fn) { \
	if (fileexists(fn)) kpw = kpw_open(fn); else kpw = NULL; \
	if (!kpw) { fprintf(stderr, "Error: \"%s\" is not a valid kpw file.\n", fn); return; } \
}

#define ppz_ensure_stats(pz) { if (!ppz_kpw_load_stats(&(pz))) { fprintf(stderr, "Error: Section STATS not found.\n"); ppz_destroy(&(pz)); return; } }
#define ppz_ensure_bwt(pz) { if (!ppz_kpw_load_bwt(&(pz))) { fprintf(stderr, "Error: Section BWT not found.\n"); ppz_destroy(&(pz)); return; } }

/* cmds */

void cmd_info(char* fn) {
	kpw_file *kpw;
	char* pt = parse_path(fn);

	kpw_open_or_die(kpw, fn);
	if (pt == fn) {
		/* KPW File info */
		fprintf(stderr, "Data size: %llu\n", kpw->noffset);
		fprintf(stderr, "Sections:\n");
		kpw_idx_it it;
		for (it = kpw_idx_begin(kpw); !KPW_IT_END(it); kpw_idx_next(&it)) {
			kpw_show_sec(it.sn, KPW_IT(it));
		}
	} else {
		kpw_idx_it it = sec_from_str(kpw, pt);
		if (KPW_IT_END(it)) {
			fprintf(stderr, "Section '%s' not found\n", pt);
		} else {
			kpw_show_sec(it.sn, KPW_IT(it));
		}
	}
	kpw_close(kpw);
}

void cmd_dump(char* fn) {
	kpw_file *kpw;
	char* pt = parse_path(fn);

	if (pt == fn) {
		fprintf(stderr, "Error: Select a section number/type (see info below)\n");
		cmd_info(fn);
	} else {
		kpw_open_or_die(kpw, fn);
		if (!kpw) fprintf(stderr, "Error: couldn't read kpw file \"%s\"\n", fn);
		kpw_idx_it it = sec_from_str(kpw, pt);
		if (KPW_IT_END(it)) {
			fprintf(stderr, "Section '%s' not found\n", pt);
		} else {
			uint64 size;
			FILE *f = kpw_idx_fileh(kpw, it, &size);
			fd_copy(stdout, f, size);
		}
		kpw_close(kpw);
	}
}

void cmd_stats(char* fn, const char* stats) {
	kpw_file *kpw;
	ppz_status pz;
	char* pt = parse_path(fn);

	if (pt != fn) fprintf(stderr, "Warning: Ignoring section selector: %s\n", pt);

	kpw_open_or_die(kpw, fn);

	ppz_create(&pz);
	/* Escribe las stats */
	pz.kpwf = kpw;
	if (!ppz_kpw_load_stats(&pz)) {
		fprintf(stderr, "Error: Couldn't read STATS section.\n");
		return;
	}
	ppz_write_stats(&pz, stats);
	pz.kpwf = NULL; // Avoid update.
	ppz_destroy(&pz);

	kpw_close(kpw);
}

/* Functions for dumping human-readable data */

#define out(...) fprintf(outf, __VA_ARGS__)

uint cmd_dumptext_trac(FILE *outf, ppz_status *pz) {
	if (!ppz_kpw_load_trac(pz)) {
		fprintf(stderr, "Error: Couldn't read TRAC section.\n");
		return 0;
	}

	uint i, start, size;
	uint acc = 0, new_acc = 0;
	uint *tb;

	int chromo = -1;
	int sep = 0;
#define nlimits 2
	uint limits[] = { 0, pz->trac_middle, };

	tb = pz->trac_buf;
	forn (i, pz->trac_size - 1) {
		new_acc = tb[1];
		size = new_acc - acc;
		acc = new_acc;
		start = tb[0] + acc - size;

		if (chromo < sep && sep < nlimits && start >= limits[sep]) {
			out("\nfile %c\n", 'A' + ++chromo);
			++sep;
			if (start != limits[sep - 1] || size != 0)
				fprintf(stderr, "Warning: TRAC section missing (0,0) gap after crossing middle boundary.\n");
			/* don't print (0,0) gap */
			continue;
		}
		out("gap %u:\tstart=\t%u,\tsize=\t%u\n", i, start - limits[sep - 1], size);
		tb += 2;

	}
	return 1;
}

#define out_stat(Name, Fmt, Descr) \
	out("# %s\n" #Name "\t" Fmt "\n", Descr, pz->data.Name)

uint cmd_dumptext_stats(FILE *outf, ppz_status *pz) {
	if (!ppz_kpw_load_stats(pz)) {
		fprintf(stderr, "Error: Couldn't read STATS section.\n");
		return 0;
	}

	out("\n");
	out_stat(rev, "%u",
	 "Source revision number of the code that generated this file");
	out_stat(hostname, "%s", "Hostname");

	out("\n### Input\n");
	out_stat(n, "%u", "Total size of the input (in bytes)");
	out_stat(nlzw, "%u", "Total size of the compressed input (in LZW-codes)");

	out("\n### Input file A\n");
	out_stat(filename_a, "%s", "Name of input file A");
	out_stat(long_source_a, "%u", "Size of input file A");
	out_stat(long_input_a, "%u", "Size of preprocessed input file A");

	out("\n### Input file B\n");
	out_stat(filename_b, "%s", "Name of input file B");
	out_stat(long_source_b, "%u", "Size of input file B");
	out_stat(long_input_b, "%u", "Size of preprocessed input file B");

	out("\n### Output\n");
	out_stat(long_output, "%u", "Size of BWT + MTF + LZW (in LZW-codes)");
	out_stat(zero_mtf, "%u", "Number of 0s in the output of BWT + MTF");
	out_stat(nzz_est, "%u", "Number of non-0 followed by 0 in the output of BWT + MTF");
	out_stat(huf_len, "%u", "Size of Huffman output (in bytes)");
	out_stat(huf_tbl_bit, "%u", "Size of Huffman table (in bits)");
	out_stat(est_size, "%u", "Estimated size of BWT + MTF + LZW + ARIT_ENC (in LZW-codes)");
	out_stat(mtf_entropy, "%u", "Estimated size of BWT + MTF + LZW + ARIT_ENC (in bytes)");
	out_stat(lzw_codes, "%u", "LZW codes");
	out_stat(lpat_size, "%llu", "Estimated size for longpat for ch=8");
	out_stat(lpat3_size, "%llu", "Estimated size for longpat for ch=3");
	out_stat(kinv_lcp, "%.3lf", "klcp");

	out("\n### Base count\n");
	out_stat(cnt_A, "%u", "Number of As");
	out_stat(cnt_C, "%u", "Number of Cs");
	out_stat(cnt_T, "%u", "Number of Ts");
	out_stat(cnt_G, "%u", "Number of Gs");
	out_stat(cnt_N, "%u", "Number of Ns");
	out_stat(N_blocks, "%u", "Number of blocks of more than 5 'N'");

	out("\n### Time stats (in ticks)\n");
	out_stat(time_bwt, "%u", "BWT");
	out_stat(time_mtf, "%u", "MTF");
	out_stat(time_lzw, "%u", "LZW");
	out_stat(time_mrs, "%u", "MRS");
	out_stat(time_mmrs, "%u", "MMRS");
	out_stat(time_huf, "%u", "Huffman");
	out_stat(time_longp, "%u", "longpat");
	out_stat(time_tot, "%u", "Total time");

	return 1;
}

uint cmd_dumptext_patt(FILE *outf, ppz_status *pz, struct options *opts) {
	/* Primero cargo las demas secciones */
	bool translate_positions = TRUE;

	fprintf(stderr, "Loading STATS section...\n");
	if (!ppz_kpw_load_stats(pz)) {
		fprintf(stderr, "Error: Couldn't read STATS section.\n");
		return 0;
	}
	fprintf(stderr, "Loading BWT section...\n");
	if (!ppz_kpw_load_bwt(pz)) {
		fprintf(stderr, "Error: Couldn't read BWT section.\n");
		return 0;
	}
	fprintf(stderr, "Loading TRAC section...\n");
	if (!ppz_kpw_load_trac(pz)) {
		fprintf(stderr, "Warning: Section TRAC not found, will not translate positions.\n");
		translate_positions = FALSE;
	}
	fprintf(stderr, "Dumping PATT section...\n");

	ppz_kpw_patt_it patt_it;

	ppz_kpw_patt_it_create(pz, &patt_it);
	uint cant_patterns, cant_occurrences;
	uint i, j, r_i, pos;
	int show, hay;

	uint virtual_middle = 0;

	if (opts->only != ONLY_ALL) {
		if (translate_positions)
			virtual_middle = trac_convert_pos_real_to_virtual(pz->trac_middle, pz->trac_buf, pz->trac_size);
		else
			virtual_middle = pz->data.long_input_a;
	}

	output_readable_data ord;
	ord.fp = outf;
	ord.r = pz->r;
	ord.s = pz->s;
	if (translate_positions) {
		ord.trac_buf = pz->trac_buf;
		ord.trac_size = pz->trac_size;
		ord.trac_middle = pz->trac_middle;
	} else {
		ord.trac_buf = NULL;
		ord.trac_size = 0;
		ord.trac_middle = pz->data.long_input_a + 1;
	}

	while ((cant_patterns = ppz_kpw_patt_next_subsection(&patt_it))) {
		if (patt_it.current_length < opts->ml) continue;
		forn (i, cant_patterns) {
			cant_occurrences = ppz_kpw_patt_next_pattern(&patt_it);
			r_i = patt_it.pattern_index;

			/* Do we have to show this pattern? */
			show = 0;
			hay = 0;
			forn(j, cant_occurrences) {
				pos = pz->r[r_i + j];
				hay |= ((pos < virtual_middle) ? ONLY_LEFT : ONLY_RIGHT);
				if (hay == ONLY_BOTH) break;
			}
			show = (opts->only == ONLY_ALL || hay == opts->only);
			if (!show) continue;

			/* If so, show it */
			output_readable_trac(patt_it.current_length, r_i, cant_occurrences, &ord);
		}
	}
	ppz_kpw_patt_it_destroy(&patt_it);
	return 1;
}

uint cmd_dumptext_lcp(FILE *outf, ppz_status *pz, struct options *opts) {
	fprintf(stderr, "Loading STATS section...\n");
	if (!ppz_kpw_load_stats(pz)) {
		fprintf(stderr, "Error: Couldn't read STATS section.\n");
		return 0;
	}
	ppz_lcp(pz, 0);
	uint i;
	forn (i, pz->data.n) {
		printf("%u\n", pz->h[i]);
	}
	return 1;
}

void cmd_dumptext(char* fn, struct options *opts) {
	kpw_file *kpw;
	char* pt = parse_path(fn);

	if (pt == fn) {
		fprintf(stderr, "Error: Select a section number/type (see info below)\n");
		cmd_info(fn);
	} else {
		kpw_open_or_die(kpw, fn);
		kpw_idx_it it = sec_from_str(kpw, pt);
		if (KPW_IT_END(it)) {
			fprintf(stderr, "Section '%s' not found\n", pt);
		} else {
			kpw_show_sec(it.sn, KPW_IT(it));

			ppz_status pz;
			ppz_create(&pz);
			pz.kpwf = kpw;

			switch (KPW_IT(it)->type) {
			case KPW_ID_STATS:
				cmd_dumptext_stats(stdout, &pz);
				break;
			case KPW_ID_TRAC:
				cmd_dumptext_trac(stdout, &pz);
				break;
			case KPW_ID_PATT:
				cmd_dumptext_patt(stdout, &pz, opts);
				break;
			case KPW_ID_LCP:
				cmd_dumptext_lcp(stdout, &pz, opts);
				break;
			default:
				fprintf(stderr, "Section %s cannot be dumped in human-readable format.\n",
						KPW_STRID(KPW_IT(it)->type));
			}
			pz.kpwf = NULL;
			ppz_destroy(&pz);
		}
		kpw_close(kpw);
	}
}

void cmd_check_fix(char* fn, int fix) {
	exit(!kpw_check_fix(fn, fix));
}

void cmd_del(char* fn) {
	kpw_file *kpw;
	char* pt = parse_path(fn);

	if (pt == fn) {
		fprintf(stderr, "Error: Select a section number/type (see info below)\n");
		cmd_info(fn);
	} else {
		kpw_open_or_die(kpw, fn);
		kpw_idx_it it = sec_from_str(kpw, pt);
		if (KPW_IT_END(it)) {
			fprintf(stderr, "Section '%s' not found\n", pt);
		} else {
			kpw_sec_delete(kpw, it);
		}
		kpw_close(kpw);
	}
}

/* Computa la LCP */
void cmd_lcp(char* fn) {
	kpw_file *kpw;
	char* pt = parse_path(fn);
	if (pt != fn) { fprintf(stderr, "Notice: Section number/type ignored\n"); }
	kpw_open_or_die(kpw, fn);
	ppz_status pz;
	ppz_create(&pz);
	pz.kpwf = kpw;
	ppz_ensure_stats(pz);
	ppz_lcp(&pz, 0);
	ppz_destroy(&pz);
}

/* Computa klcp */
void cmd_klcp(char* fn) {
	kpw_file *kpw;
	char* pt = parse_path(fn);

	if (pt != fn) { fprintf(stderr, "Notice: Section number/type ignored\n"); }
	kpw_open_or_die(kpw, fn);
	ppz_status pz;
	ppz_create(&pz);
	pz.kpwf = kpw;
	ppz_ensure_stats(pz);

	/* Calcula klcp */
	ppz_klcp(&pz, 1);

	ppz_destroy(&pz);
}

void cmd_mtf(char *fn) {
	kpw_file *kpw;
	char* pt = parse_path(fn);
	if (pt != fn) { fprintf(stderr, "Notice: Section number/type ignored\n"); }
	kpw_open_or_die(kpw, fn);
	ppz_status pz;
	ppz_create(&pz);
	pz.kpwf = kpw;
	ppz_ensure_stats(pz);
	ppz_ensure_bwt(pz);
	ppz_mtf(&pz);
	ppz_kpw_save_stats(&pz);
	ppz_destroy(&pz);
}

#define MAX_ARGV 32
int main(int argc, char* argv[]);
void cmd_cmd(char* fn) {
	uint n = 0;
	char* fc = (char*)loadStrFileExtraSpace(fn, &n, 1);
	char* it;
	if (!fc) return;
	fc[n] = 0;
	char* argv[MAX_ARGV];
	argv[0] = "kapow";
	
	for(it = fc; *it; ) {
		uint argc = 1;
		while (*it == ' ' || *it == '\n' || *it == '\r') it++;
		argv[argc++] = it;
		do {
			if (*it == ' ') { 
				*it = 0; it++;
				while (*it == ' ') it++;
				argv[argc++] = it;
				if (*it == '\n' || *it == '\r') argc--;
			} else it++;
		} while (*it && (*it != '\n') && (*it != '\r'));
		while (*it == '\n' || *it == '\r') *(it++) = 0;
//		uint i; forn(i, argc) fprintf(stderr, "argv[%d] = «%s»\n", i, argv[i]);
		if (argc > 1) main(argc, argv);
	}
	pz_free(fc);
}

void usage() {
	fprintf(stderr, "KAPOW file manager - rev%d\n", SVNREVISION);
	fprintf(stderr,
	  "Usage:\n"
	  "  kapow <subcommand> [options] file.kpw[:section]\n"
	  "\n"
	  "Available subcommands:\n"
	  "(-/+ before the option activates/deactivates the option)\n"
	  "\n"
	  "  cmd <file>\t\tRead command options from <file> instead of command line, one per line.\n"
	  "  info (i)\n"
	  "  dump (d)\t\tDump the section\n"
	  "  dumptext (dt)\t\tDump the section in human-readable format\n"
	  "        L | R | 2\tPrint only patterns in left/right/both file(s) (use only one of these) (default is print all).\n"
	  "        --ml <value>\n"
	  "  check\t\t\tCheck file integrity\n"
	  "  fix\t\t\tCheck file integrity and repair when possible\n"
	  "  stats (s)\n"
	  "  del\n"
	  "  lcp\t\t\tCalculate and store the LCP section\n"
	  "  klcp\t\tCalculate the complexity of the input using inverse LCP \n"
	  "\n"
	  "Example:\n"
	  "  kapow info somefile.kpw\n"
	  "  kapow dumptext -L somefile.kpw:PATT\n"
	);
}

#define subcommand(S)  if (!strcmp(subcmd, S))

int main(int argc, char* argv[]) {
	struct options opts;

	uint i;

	if (argc <= 1) {
		usage();
		return 1;
	}

	char *subcmd = argv[1];
	char *filename = NULL;

	int flagL = 0, flagR = 0, flag2 = 0;
	opts.ml = 0;

	/* Read options */
	forsn(i, 2, argc) {
		if (0);
		else cmdline_var(i, "2", flag2)
		else cmdline_var(i, "L", flagL)
		else cmdline_var(i, "R", flagR)
		else cmdline_opt_2(i, "--ml") { opts.ml = atoi(argv[i]); }
		else if (cmdline_is_opt(i)) {
			fprintf(stderr, "%s: Unknown option: %s\n", argv[0], argv[i]);
			usage();
			return 1;
		}
		else { // Default parameter
			filename = argv[i];
		}
	}

	if (!filename || flagL + flagR + flag2 > 1) {
		usage();
		return 1;
	}

	opts.only = ONLY_ALL;
	if (0);
	else if (flagL) opts.only = ONLY_LEFT;
	else if (flagR) opts.only = ONLY_RIGHT;
	else if (flag2) opts.only = ONLY_BOTH;

	/* Dispatch to the corresponding subcommand */
	if (0);
	else subcommand("cmd") cmd_cmd(filename);

	else subcommand("i") cmd_info(filename);
	else subcommand("info") cmd_info(filename);

	else subcommand("check") cmd_check_fix(filename, 0);

	else subcommand("fix") cmd_check_fix(filename, 1);

	else subcommand("d") cmd_dump(filename);
	else subcommand("dump") cmd_dump(filename);

	else subcommand("dt") cmd_dumptext(filename, &opts);
	else subcommand("dumptext") cmd_dumptext(filename, &opts);

	else subcommand("s") cmd_stats(filename, "kapow.txt");
	else subcommand("stats") cmd_stats(filename, "kapow.txt");

	else subcommand("del") cmd_del(filename);

	else subcommand("lcp") cmd_lcp(filename);
	else subcommand("klcp") cmd_klcp(filename);

	else subcommand("mtf") cmd_mtf(filename);
	else { // Default parameter
		fprintf(stderr, "%s: Unknown option: %s\n", argv[0], argv[i]);
		usage();
		return 1;
	}

	return 0;
}

