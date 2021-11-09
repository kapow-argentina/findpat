#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <assert.h>

#include "tipos.h"
#include "common.h"
#include "macros.h"
#include "libp2zip.h"
#include "output_callbacks.h"

void lookup_pattern(uint *r, uchar *s, uint n, uchar *pat, uint len, uint *lu_array, uint lcp,
			output_callback out, void* out_data);
/* lu_array:
 *
 * An array that holds the indexes in "r" for the each prefix of the line.
 *
 *   lu_array[2 * k]     = min { i | s[r[i]..r[i]+k-1] == line[0..k-1] } = lower index
 *   lu_array[2 * k + 1] = max { i | s[r[i]..r[i]+k-1] == line[0..k-1] } = upper index
 *
 * E.g.
 *
 *   line = "ABCBDB"
 *
 *   lu_array[0] = 0
 *   lu_array[1] = n    (size of r)
 *
 *   lu_array[2] = min { i | s[r[i]] == "A" }
 *   lu_array[3] = max { i | s[r[i]] == "A" } + 1
 *
 *   lu_array[4] = min { i | s[r[i]..r[i]+1] == "AB" }
 *   lu_array[5] = max { i | s[r[i]..r[i]+1] == "AB" } + 1 
 *
 *   ...
 */

#define lu_array_size(X) 	(2 * (X) * sizeof(uint))
void process_file_lines(uint *r, uchar *s, uint n, FILE *inf, output_callback out, void *out_data) {
#define tam_buf		8192
	static uchar buf[tam_buf];

	uint line_size = tam_buf;

	uchar *line = pz_malloc(line_size);
	bzero(line, line_size);

	uint i = 0; /* buffer */
	uint j = 0; /* line */

	uint *lu_array = pz_malloc(lu_array_size(line_size));
	uint lcp = 0;
	bool reading_common_pref = TRUE;

	/* process each line */
	for (;;) {
		uint rd = fread(buf, sizeof(uchar), tam_buf, inf);
		forn (i, rd) {
			if (j >= line_size) {
				/* Handle the growth of the line beyond the
				 * current size of the "line" buffer */
				uint new_line_size = 2 * line_size;
				uchar *new_line = pz_malloc(new_line_size);
				bzero(new_line, new_line_size);
				memcpy(new_line, line, line_size);
				pz_free(line);
				line = new_line;

				/* Also grow the lower-upper array */
				uint *new_lu_array = pz_malloc(lu_array_size(new_line_size));
				memcpy(new_lu_array, lu_array, lu_array_size(line_size));
				pz_free(lu_array);
				lu_array = new_lu_array;

				line_size = new_line_size;
			}

			if (buf[i] == '\n') {
				/* Process line */
				line[j] = '\0';
				lookup_pattern(r, s, n, line, j, lu_array, lcp, out, out_data);
				j = 0;
				reading_common_pref = TRUE;
				lcp = 0;
				continue;
			}

			/* keep the LCP of the current line vs. the previous one */
			if (reading_common_pref) {
				if (buf[i] == line[j]) ++lcp;
				else reading_common_pref = FALSE;
			}
			line[j++] = buf[i];
		}
		if (rd < tam_buf) break;
	}
	pz_free(lu_array);
	pz_free(line);
}

#define midpoint(_L, _U)	(((_L) >> 1) + ((_U) >> 1) + (1 & (_L) & (_U)))
#define IMP(x, y) (!(x) || (y))
void lookup_pattern(uint *r, uchar *s, uint n, uchar *pat, uint len, uint *lu_array, uint lcp,
			output_callback out, void* out_data) {
/* Update L, U to L', U' so that L' == U' + 1, L <= L', U' <= U and
 *   U' is the leftmost index s.t. s[RI(U')] not[OP] VAL if there is any.
 *   U' == U  if not
 */
#define binsearch(L, U, RI, VAL, OP) { \
	while (L + 1 < U) { \
		uint _mid = midpoint(L, U); \
		if (RI(_mid) < n && s[RI(_mid)] OP VAL) L = _mid; \
		else U = _mid; \
	} \
}
	uint lower, upper, i;
	uint nu, nl, val;

	if (lcp == 0) {
		lower = 0;
		upper = n;
	} else {
		lower = lu_array[2 * (lcp - 1)];
		upper = lu_array[2 * (lcp - 1) + 1];
	}
	for (i = lcp; lower < upper && i < len; ++i) {
#define _ri_izq(x) (r[(x) - 1] + (i))
#define _ri_der(x) (r[(x)] + (i))
		/* Find the new boundaries */
		val = pat[i];
		if (s[r[lower] + i] > val || s[r[upper - 1] + i] < val) {
			lower = upper;
		} else {
			nu = upper, nl = lower;
			binsearch(lower, nu, _ri_izq, val, <);
			binsearch(nl, upper, _ri_der, val, <=);
		}

		/* Save the values of lower and upper
		 * for the current length of the prefix */
		lu_array[2 * i] = lower;
		lu_array[2 * i + 1] = upper;
#undef _ri_izq
#undef _ri_der
	}

	if (lower < upper) {
		/* Report the occurrences */
		out(len, lower, upper - lower, out_data);
	} else {
		/* Pattern not found */
		assert(lower == upper);
		for (; i < len; ++i) {
			lu_array[2 * i] = lower;
			lu_array[2 * i + 1] = upper;
		}
	}
}

int main(int argc, char **argv) {
	FILE *inf;
	char *kpwfn, *patfn;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <file.kpw> <pats.txt>\n", argv[0]);
		fprintf(stderr, "  pats.txt : one string per line (use \"-\" for stdin)\n");
		return 1;
	}

	kpwfn = argv[1];
	patfn = argv[2];

	kpw_file *kpw = kpw_open(kpwfn);
	if (!kpw) {
		fprintf(stderr, "Error: \"%s\" is not a valid kpw file.\n", kpwfn);
		exit(1);
	}

	if (!strcmp(patfn, "-")) {
		inf = stdin;
	} else if (!(inf = fopen(patfn, "r"))) {
		fprintf(stderr, "Error: cannot open file: \"%s\"\n", patfn);
		exit(1);
	}

	ppz_status pz;
	ppz_create(&pz);
	pz.kpwf = kpw;
	fprintf(stderr, "Loading STATS section...\n");
	if (!ppz_kpw_load_stats(&pz)) {
		fprintf(stderr, "Error: cannot load STATS section.\n");
		exit(1);
	}
	fprintf(stderr, "Loading BWT section...\n");
	if (!ppz_kpw_load_bwt(&pz)) {
		fprintf(stderr, "Error: cannot load BWT section.\n");
		exit(1);
	}
	fprintf(stderr, "Loading TRAC section...\n");
	if (!ppz_kpw_load_trac(&pz)) {
		fprintf(stderr, "Warning: cannot load TRAC section.\n");
		pz.trac_buf = 0;
		pz.trac_middle = pz.trac_size = 0;
	}

	output_readable_data out_opts;
	out_opts.r = pz.r;
	out_opts.s = pz.s;
	out_opts.fp = stdout;
	out_opts.trac_buf = pz.trac_buf;
	out_opts.trac_size = pz.trac_size;
	out_opts.trac_middle = pz.trac_middle;

	process_file_lines(pz.r, pz.s, pz.data.n, inf, output_readable_trac, &out_opts);

	if (inf != stdin) fclose(inf);
	ppz_destroy(&pz);
	return 0;
}

