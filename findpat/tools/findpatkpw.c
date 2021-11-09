#include <ctype.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include "tipos.h"
#include "libp2zip.h"
#include "common.h"
#include "tiempos.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>

#include <assert.h>

#include "output_callbacks.h"
#include "mmrs.h"
#include "lcp.h"
#include "macros.h"

#define MAX_LEN 1024

int main(int argc, char **argv) {
	char *xargv[4];
	int xargc = 0;
	int i;
	int ml = 1;

	int opt_bwt = 0, opt_mmrs = 0, opt_nolcp = 0, opt_mtf = 0, opt_sep = 0;

	ppz_status pz;
	ppz_create(&pz);

	pz.fa_input = 1;
	pz.fa_strip_n = 1;
	pz.use_terminator = 1;
	pz.use_separator = 1;
	pz.trac_positions = 1;

	forsn(i, 1, argc) {
		if (0);
		else cmdline_var(i, "z", pz.gz_input)
		else cmdline_var(i, "f", pz.fa_input)
		else cmdline_var(i, "r", pz.trac_positions)
		else cmdline_opt_1(i, "--bwt") { opt_bwt = 1; }
		else cmdline_opt_1(i, "--mmrs") { opt_mmrs = 1; }
		else cmdline_opt_1(i, "--mtf") { opt_mtf = 1; }
		else cmdline_opt_1(i, "--sep") { opt_sep = 1; }
		else cmdline_opt_1(i, "--no-strip-n") { pz.fa_strip_n = 0; }
		else cmdline_opt_1(i, "--nolcp") { opt_nolcp = 1; }
		else if (cmdline_is_opt(i)) {
			fprintf(stderr, "%s: Unknown option: %s\n", argv[0], argv[i]);
			return 1;
		} else {
			if (xargc == 4) {
				fprintf(stderr, "%s: Too many arguments: %s\n", argv[0], argv[i]);
				return 1;
			}
			xargv[xargc++] = argv[i];
		}
	}

	if (xargc == 4) {
		ml = atoi(xargv[3]);
	}
	if (xargc != 4 || ml <= 0) {
		fprintf(stderr, "Use: %s [options] <input1> <input2> <output> <min_length>\n", argv[0]);
		fprintf(stderr, "Options: (-/+ before the option activates/deactivates the option)\n"
			"  f        Treats the input as FASTA files when possible (default is on).\n"
			"  z        Treats the input as a compressed gzip file. If it's not compressed ignores this flag.\n"
			"  r        Show real positions of patterns long lists of Ns (default is on)\n"
			"\n"
			" Additional options:\n"
			"  --bwt    Stop after BWT phase, don't calculate patterns\n"
			"  --mmrs   Calculate MMRS instead of MRS\n"
			"  --nolcp  Use the variant of MRS that overwrites the LCP with its inverse\n"
			"  --mtf    Calculate zero_mtf\n"
			"  --sep    Use '~' as an LCP separator\n"
		);
		return 1;
	}

#ifdef _ENABLE_LCP_SEPARATOR
	if (!opt_sep) {
		fprintf(stderr, "Error: LCP separator enabled in compilation, must use --sep option\n");
		exit(1);
	}
#else
	if (opt_sep) {
		fprintf(stderr, "Error: LCP separator not enabled in compilation, cannot use --sep option\n");
		exit(1);
	}
#endif
	
	if (opt_bwt && opt_mmrs) {
		fprintf(stderr, "Warning: ignoring --mmrs option.");
	}

	if (!ppz_kpwfile_load(&pz, xargv[2], xargv[0], xargv[1])) {
		fprintf(stderr, "Could not find or create KPW file.\n");
		exit(1);
	}

	if (opt_mtf) {
		ppz_mtf(&pz);
		ppz_mtf_ests(&pz);
		if (pz.bwto) { pz_free(pz.bwto); pz.bwto = NULL; }
		ppz_klcp(&pz, 1);
	}

	if (opt_bwt) goto end_main;

	tiempo t1, t2;
	getTickTime(&t1);

	if (pz.st != PZ_ST_BWT && pz.st != PZ_ST_MTF) {
		fprintf(stderr, "%s", "ERROR, files not loaded.\n");
		exit(1);
	}

	if (!opt_mmrs) {
		fprintf(stderr, "Calculating MRS\n");
		if (opt_nolcp) {
			ppz_mrs_nolcp(&pz, 1, ml);
		} else {
			ppz_mrs(&pz, 1, ml);
		}
		getTickTime(&t2);
		pz.data.time_tot += (uint)(getTimeDiff(t1, t2));
		pz.update_data = TRUE;
	} else {
		fprintf(stderr, "Calculating MMRS\n");
		ppz_mmrs(&pz, 1, ml);
	}

end_main:
	ppz_destroy(&pz);
	return 0;
}

