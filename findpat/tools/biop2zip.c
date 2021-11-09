#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tipos.h"
#include "libp2zip.h"
#include "common.h"

#include "macros.h"

#define BWT 0
#define OBWT 1

int files_gz=1;
int files_fa=1;
int strip_n=1;
char* save_bwt = NULL;

void compress_bin(const char* in1, const char* in2, const char* out, int bwtf) {
	ppz_status pz;
	ppz_create(&pz);
	pz.fa_input = files_fa;
	pz.gz_input = files_gz;
	pz.fa_strip_n = strip_n;
	pz.use_terminator = 1;
	pz.use_separator = 1;
	pz.trac_positions = 1;
	ppz_kpw_open(&pz, out);
	ppz_load_file2(&pz, in1, in2);
	/* TODO: Implementar que guarde la BWT si save_bwt != NULL */
	switch(bwtf) {
		case BWT:
			ppz_compress(&pz);
			break;
		case OBWT:
			ppz_ocompress(&pz);
			break;
//		default:
//			ppz_compress(&pz);
	}
	ppz_write_stats(&pz, "biop2zip.txt");
	ppz_destroy(&pz);
}

int main(int argc, char* argv[]) {
	fprintf(stderr, "biop2zip - rev%d\n", SVNREVISION);

	char* files[3], *out;
	int nf = 0, i, bwtf = BWT;
	forn(i, 3) files[i] = NULL;

	forsn(i, 1, argc) {
		if (0);
		else cmdline_var(i, "z", files_gz)
		else cmdline_var(i, "f", files_fa)
		else cmdline_opt_1(i, "--obwt") { bwtf = OBWT; }
		else cmdline_opt_2(i, "--bwt") { save_bwt = argv[i]; }
		else cmdline_opt_2(i, "--no-strip-n") { strip_n = 0; }
		else if (cmdline_is_opt(i)) {
				fprintf(stderr, "%s: Opción desconocida: %s\n", argv[0], argv[i]);
				return 1;
		}
		else { // Default parameter
			if (nf == 3) {
				fprintf(stderr, "%s: Demasiados par'ametros: %s\n", argv[0], argv[i]);
				return 1;
			}
			files[nf++] = argv[i];
		}
	}

	if (nf < 1) {
		fprintf(stderr, "Use: %s [options] <input1> [<input>] [output_dir]\n", argv[0]);
		fprintf(stderr, "Options: (-/+ before the option activates/deactivates the option)\n"
			"  f    Treats the input as FASTA files when possible.\n"
			"  z    Treats the input as a compressed gzip file. If it's not compressed ignores this flag.\n"
		);
		return 1;
	}

	out = std_name(files[2], NULL, files[0], files[1], "kpw");
	compress_bin(files[0], files[1], out, bwtf);
	pz_free(out);

	return 0;
}
