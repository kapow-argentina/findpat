#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bwt.h"
#include "lcp.h"
#include "cop.h"
#include "tipos.h"
#include "common.h"
#include "macros.h"
#include "mmrs.h"
#include "output_callbacks.h"
#include "mrs.h"
#include "libp2zip.h"

void _set_pz_options(ppz_status *pz) {
	/* Default options in case we need to compute a .kpw file */
	pz->fa_input = 1;
	pz->fa_strip_n = 1;
	pz->use_terminator = 1;
	pz->use_separator = 1;
	pz->trac_positions = 1;
}

void show_bwt_lcp(uint n, uchar* src, uint* r, uint* h) {
	int i, j;
	printf("\n");
	printf(" i  r[] lcp \n");
	forn(i, n) {
		printf("%3d %3d %3d ", i, r[i], h[i]);
		forn(j, n) printf("%c", src[(r[i]+j)%n]);
		printf("\n");
	}
}

bool check_files(char *path, char *file0, uint at, char **rest) {
	char name[PPZ_MAX_KPW_FILENAME];
	uint i;
	forn (i, at + 1) {
		char *o = (i == 0 ? "/dev/null" : rest[i - 1]);
		ppz_status pz;
		ppz_create(&pz);
		_set_pz_options(&pz);
		if (!ppz_kpwfile_find(&pz, name, path, file0, o)) {
			fprintf(stderr, "ERROR: Couldn't create KPW file for <%s>, <%s>\n", file0, o);
			return FALSE;
		}
		ppz_destroy(&pz);
	}
	return TRUE;
}

#define DEBUG_COP	1

int load_data(char *path, char *file, ppz_status *pz) {
	char name[PPZ_MAX_KPW_FILENAME];
	int found = ppz_kpwfile_find(pz, name, path, file, "/dev/null");
	if (found != PPZ_LEFT && found != PPZ_NEW) {
		fprintf(stderr, "ERROR: Couldn't find KPW file for <%s>\n", file);
		return FALSE;
	}
	if (!ppz_kpw_load_stats(pz)) {
		fprintf(stderr, "Cannot load DATA section for <%s>\n", file);
		return FALSE;
	}
	kpw_close(pz->kpwf);
	pz->kpwf = NULL;
	return PPZ_LEFT;
}

/* XXX: Happily leaky */
uint read_string_list(char *filename, char ***strings) {
	uint i, j, cant = 0;
	uint sz = filesize(filename);
	char *whole_file = pz_malloc(sz);
	FILE *f = fopen(filename, "r");
	if (!f || fread(whole_file, sizeof(char), sz, f) != sz) {
		fprintf(stderr, "Error: cannot read from %s\n", filename);
		return 0;
	}
	fclose(f);
	for (i = 0; i < sz; ++i) if (whole_file[i] == '\n') ++cant;
	*strings = pz_malloc(sizeof(char *) * cant);
	char *last_string_start = whole_file;
	j = 0;
	for (i = 0; i < sz; ++i) {
		if (whole_file[i] == '\n') {
			whole_file[i] = '\0';
			(*strings)[j++] = last_string_start;
			last_string_start = &whole_file[i + 1];
		}
	}
	return cant;
}

/* Functions to dump M to make graphics */

void dump_header(FILE *f, uint n, uint c) {
	if (fwrite(&n, sizeof(uint), 1, f) != 1) perror("writing dump file");
	if (fwrite(&c, sizeof(uint), 1, f) != 1) perror("writing dump file");
}

void dump_array(FILE *f, uint n, uint *m) {
	if (fwrite(m, sizeof(uint), n, f) != n) perror("writing dump file");
}

int main(int argc, char** argv) {
	char *file0 = NULL;
	char **rem_files, **mem_to_free;
	uint num_checkpoints = 0;
	bool *checkpoint = NULL;
	uint i, file0_n;
	uint ml = 1;
	uint nm = 0;
	uint c = 0;
	uint *m, *mc;

	char *list_filename = NULL;

	FILE *dump_m_file = NULL;

	char *base_path = ".";

	uint nitems = 0;
	int ps = -1, at = -1;
	/* Read command line arguments */
	forsn(i, 1, argc) {
		if (0) {}
		else cmdline_opt_2(i, "-ml") { ml = atoi(argv[i]); }
		else cmdline_opt_2(i, "-f") { list_filename = argv[i]; }
		else cmdline_opt_2(i, "-bp") { base_path = argv[i]; }
		else cmdline_opt_2(i, "-mdump") {
			if (!(dump_m_file = fopen(argv[i], "w"))) {
				fprintf(stderr, "cannot open %s\n", argv[i]);
				perror("opening dump file");
			}
		}
		else cmdline_var(i, "nm", nm)
		else cmdline_var(i, "c", c)
		else {
			if (ps == -1) ps = i;
			nitems++;
		}
	}
	if ((!list_filename && nitems < 1) || (nm && c)) {
		fprintf(stderr, "Usage: %s <file> <file1> [<file2>] [<file3>]"
						" ... [-ml ml] [-nm] \n", argv[0]);
		fprintf(stderr, "Usage: %s -f <list-file> [-ml ml] [-nm]\n", argv[0]);
		fprintf(stderr, "  -nm will run mrs instead of mmrs\n"
				"  -ml <number> will use <number> as ml parameter\n"
				"  -c will find common patterns instead of own (default)\n"
				"  -bp <path> uses <path> as directory to look for .kpw files\n");
		fprintf(stderr, "  -mdump <file> dumps the intermediate versions of the M array in the given file\n"
				"                indicate checkpoints for dumping with \"--\"\n"
				"                e.g. %s <file> <file1> <file2> -- <file3> --\n", argv[0]);
		return 1;
	}

	char **rest;
	if (list_filename) {
		nitems = read_string_list(list_filename, &mem_to_free);
		file0 = mem_to_free[0];;
		rest = &mem_to_free[1];
	} else {
		file0 = argv[ps];
		rest = &argv[ps + 1];
	}

	--nitems;
	at = 0;
	forn (i, nitems) {
		if (strcmp(rest[i], "--") != 0) at++;
	}
	rem_files = mem_to_free = (char **)pz_malloc(at * sizeof(char*));
	checkpoint = (bool *)pz_malloc(at * sizeof(bool));
	if (!strcmp(rest[0], "--")) {
		fprintf(stderr, "Error: cannot put a checkpoint after the first file.\n");
		exit(1);
	}
	uint fst = 0;
	forn(i, at) {
		if (fst >= nitems) break;
		rem_files[i] = rest[fst++];
		checkpoint[i] = 0;
		while (fst < nitems && !strcmp(rest[fst], "--")) {
			fst++;
			checkpoint[i] = 1;
		}
		if (checkpoint[i]) num_checkpoints++;
	}

	/* Check that all required files exist, or create them */
	check_files(base_path, file0, at, rem_files);

	/* Initialize */
	fprintf(stderr, "--- Initializing\n");

	ppz_status pz;
	ppz_create(&pz);

	_set_pz_options(&pz);

	if (!load_data(base_path, file0, &pz)) exit(1);
	/* pz.data.n should be (pz.data.long_input_a + two separators) */
	assert(pz.data.long_input_a + 1 == pz.data.n - 1);
	file0_n = pz.data.long_input_a + 1;

	m = (uint *)pz_malloc(file0_n * sizeof(uint));

	mc = (uint *)pz_malloc(file0_n * sizeof(uint));
	if (c) {
		forn(i, file0_n) mc[i] = file0_n;
	} else {
		forn(i, file0_n) mc[i] = 0;
	}

	if (dump_m_file) dump_header(dump_m_file, file0_n, num_checkpoints);
	
	/* Process file0 vs. rem_files[i] */
	forn (i, at) {
		fprintf(stderr, "--- %s vs. %s\n", file0, rem_files[i]);
		int loaded = ppz_kpwfile_load(&pz, base_path, file0, rem_files[i]);
		if (!loaded) exit(1);
		/* Calculate the LCP */
		uint *pz_h = (uint*)pz_malloc(pz.data.n * sizeof(uint));
		memcpy(pz_h, pz.r, pz.data.n * sizeof(uint));
		lcp(pz.data.n, pz.s, pz_h, pz.p);

		if (loaded == PPZ_LEFT)
			mcl(pz.r, pz_h, pz.data.n, m, file0_n);
		else {
			assert(loaded == PPZ_RIGHT);
			mcl_reverse(pz.r, pz_h, pz.data.n, m, file0_n);
		}

		if (c) csu(mc, m, file0_n);
		else opu(mc, m, file0_n);

		if (dump_m_file && checkpoint[i]) dump_array(dump_m_file, file0_n, mc);

		pz_free(pz_h);
		/* pz.r, pz.p, ps.s are already freed by libp2zip */
	}

	if (dump_m_file && fclose(dump_m_file)) perror("closing dump file");

	fprintf(stderr, "--- %s\n", file0);
	if (!ppz_kpwfile_load(&pz, base_path, file0, "/dev/null")) exit(1);

	assert(file0_n == pz.data.long_input_a + 1);
	assert(file0_n == pz.data.n - 1);

	uint *pz_h = (uint *)pz_malloc(pz.data.n * sizeof(uint));
	memcpy(pz_h, pz.r, pz.data.n * sizeof(uint));
	lcp(file0_n, pz.s, pz_h, pz.p);

	output_callback *callback = output_readable_po;

	output_readable_data ord;
	ord.r = pz.r;
	ord.s = pz.s;
	ord.fp = stdout;

	if (!c) {
		filter_data fdata;
		fdata.data = (void*) &ord;
		fdata.filter = mc;
		fdata.r = pz.r;
		fdata.callback = callback; 

		if (nm) {
			fprintf(stderr, "--- MRS\n");
			mrs(pz.s, file0_n, pz.r, pz_h, pz.p, ml, own_filter_callback, &fdata);	
		} else {
			fprintf(stderr, "--- MMRS\n");
			mmrs(pz.s, file0_n, pz.r, pz_h, ml, own_filter_callback, &fdata);	
		}
	} else {
		fprintf(stderr, "--- Common substrings\n");
		common_substrings(pz.s, file0_n, pz.r, mc, pz_h, ml, callback, &ord);
	}

	pz_free(mem_to_free);
	pz_free(mc);
	pz_free(m);
	pz_free(pz_h);
	ppz_destroy(&pz);

	return 0;
}
