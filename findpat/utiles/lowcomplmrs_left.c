#define _BSD_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>

#include "macros.h"
#include "common.h"
#include "sorters.h"
#include "enc.h"
#include "bwt.h"
#include "lcp.h"
#include "bitarray.h"
#include "bittree.h"
#include "nsrph.h"

static __thread uint* data;
#define DATA_VAL(x) data[*(x)]
_def_qsort3(_qsort_data, uint, uint, DATA_VAL, >)

static void sort(uint n, uint* _data, uint *ind) {
	uint i;
	data = _data;
	forn(i,n) ind[i] = i;
	_qsort_data(ind, ind + n);
}

uint *height_array(uint *h, uint n, uint *height) {
	uint i;
	forn (i, n) {
		uint j = 0;
		uint lower = i;
		while (lower > 0 && h[lower - 1] >= h[i]) lower--;

		uint upper = i + 2;
		while (upper < n && h[upper - 1] >= h[i]) upper++;

		height[i] = upper - lower;
	}
}

enum OPTIONS { LIMLOG, LIMLCP, LIM1,
		REPLACE_NONE, REPLACE_OPPOSITE, REPLACE_COMMON, REPLACE_RANDOM,
		PNT_AREA, PNT_BASE, PNT_ALTURA,
		RANGO_GT_LOG, RANGO_EQ_LOGM1, RANGO_EQ_HALFLOG, RANGO_CTE, };
uint option_lim = LIMLOG;
uint option_replace = REPLACE_NONE;
uint option_puntaje = PNT_AREA;
uint option_rango = RANGO_GT_LOG;

bool en_rango(uint h, uint log4n) {
	if (option_rango == RANGO_GT_LOG) {
		return h >= log4n;
	} else if (option_rango == RANGO_EQ_LOGM1) {
		return h == log4n - 1;
		//return h >= 2 * log4n;
	} else if (option_rango == RANGO_EQ_HALFLOG) {
		return h == log4n / 2;
	} else if (option_rango == RANGO_CTE) {
		return h == 10;
	} else {
		assert(FALSE);
		return 0;
	}
}

char rand_base() {
	uint b = (uint)(floor((((double)random()) / ((double)RAND_MAX + 1)) * 4));
	switch (b) {
	case 0: return 'A';
	case 1: return 'C';
	case 2: return 'G';
	case 3: return 'T';
	}
}

void lowcompl(uint n, uchar *s, uint *r, uint *h, uint *height) {
	uint i;
	uint *puntaje = height;
	uint log4n = ceil(log(n) / log(4));

	forn (i, n) {
		// calcular puntaje:
		//   ancho * alto del rectángulo
		//
		// siempre y cuando
		//  - no se vaya de rango
		//  - lcp supere cota
		if (en_rango(h[i], log4n)) {
			if (option_puntaje == PNT_AREA) {
				if (h[i] < 0xffff && height[i] < 0xffff) {
					puntaje[i] = h[i] * height[i];
				} else {
					puntaje[i] = 0x7fffffff;
				}
			} else if (option_puntaje == PNT_BASE) {
				puntaje[i] = h[i];
			} else if (option_puntaje == PNT_ALTURA) {
				puntaje[i] = height[i];
			}
		} else {
			puntaje[i] = 0;
		}
	}

	// ind: permutacion que ordena los puntajes de mayor a menor
	uint *ind = (uint *)pz_malloc(n * sizeof(uint));
	sort(n - 1, puntaje, ind);

	// indices del string que ya fueron visitados
	bitarray *usado = bita_malloc(n);
	bita_clear(usado, n);

	uint ii;
	forn (ii, n) {
		i = ind[ii];
		//if (puntaje[i] == 0) break;
		//if (puntaje[i] <= log4n * log4n) break;

		//fprintf(stderr, " %u\n", puntaje[i]);

		if (bita_get(usado, r[i])) continue;
		if (bita_get(usado, r[i + 1])) continue;

		uint lower = i;
		//while (lower > 0 && h[lower - 1] >= h[i] && !bita_get(usado, r[lower - 1]) && puntaje[lower - 1] != 0) lower--;
		while (lower > 0 && h[lower - 1] >= h[i] && !bita_get(usado, r[lower - 1])) lower--;

		uint upper = i + 2;
		//while (upper < n && h[upper - 1] >= h[i] && !bita_get(usado, r[upper]) && puntaje[upper]) upper++;
		while (upper < n && h[upper - 1] >= h[i] && !bita_get(usado, r[upper])) upper++;

		//printf("%u %u (%u) [%u -- %u]\n", r[i], h[i], puntaje[i], lower, upper);

#define OppositeBase(B) ( \
		B == 'A' ? 'T' : \
		B == 'T' ? 'A' : \
		B == 'C' ? 'G' : \
		B == 'G' ? 'C' : 0 )
		uint j, k;

		char replace = 0;

		if (option_replace == REPLACE_NONE) { replace = 0; }
		else if (option_replace == REPLACE_OPPOSITE) { replace = OppositeBase(s[r[lower]]); }
		else if (option_replace == REPLACE_COMMON) {
			char cnt[256];
			forn (j, 256) cnt[j] = 0;
			forsn (j, lower, upper) {
				if (r[j] == 0) continue;
				cnt[s[r[j] - 1]]++;
			}
			uint mx = 0;
			forn (j, 256) {
				if (cnt[j] > mx) {
					mx = cnt[j];
					replace = j;
				}
			}
		} else if (option_replace == REPLACE_RANDOM) {
			replace = rand_base();
		}
		forsn (j, lower, upper) {
			if (r[j] != 0) {
				bita_set(usado, (r[j] - 1) % n);
				if (!replace) {
					replace = s[r[j] - 1];
				} else {
					s[r[j] - 1] = replace;
				}
			}
			uint limit = log4n;

			if (option_lim == LIMLOG) { limit = log4n; }
			else if (option_lim == LIMLCP) { limit = h[i]; }
			else if (option_lim == LIM1) { limit = 1; }

			forn (k, limit) {
				bita_set(usado, (r[j] + k) % n);
			}
		}
	}
	free(usado);
	free(ind);
}

int main(int argc, char **argv) {

	srandom(time(NULL));

	if (argc < 2) {
		fprintf(stderr, "Usage: %s file.fa[.gz] [options]\n", argv[0]);
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "   limlog, limlcp, lim1\n");
		fprintf(stderr, "   rany, ropposite, rcommon, rrandom\n");
		fprintf(stderr, "   parea, pbase, paltura\n");
		fprintf(stderr, "   rg_gt_log, rg_eq_logm1, rg_eq_halflog, rg_cte\n");
		fprintf(stderr, "   i<iterations>\n");
		return 1;
	}
	char *fn = argv[1];

	uint TIMES = 1;
	uint i;
	forsn (i, 2, argc) {
		if (0) {
		} else if (!strcmp(argv[i], "limlog")) {
			option_lim = LIMLOG;
		} else if (!strcmp(argv[i], "limlcp")) {
			option_lim = LIMLCP;
		} else if (!strcmp(argv[i], "lim1")) {
			option_lim = LIM1;
		} else if (!strcmp(argv[i], "rany")) {
			option_replace = REPLACE_NONE;
		} else if (!strcmp(argv[i], "ropposite")) {
			option_replace = REPLACE_OPPOSITE;
		} else if (!strcmp(argv[i], "rcommon")) {
			option_replace = REPLACE_COMMON;
		} else if (!strcmp(argv[i], "rrandom")) {
			option_replace = REPLACE_RANDOM;
		} else if (!strcmp(argv[i], "parea")) {
			option_puntaje = PNT_AREA;
		} else if (!strcmp(argv[i], "pbase")) {
			option_puntaje = PNT_BASE;
		} else if (!strcmp(argv[i], "paltura")) {
			option_puntaje = PNT_ALTURA;
		} else if (!strcmp(argv[i], "rg_gt_log")) {
			option_rango = RANGO_GT_LOG;
		} else if (!strcmp(argv[i], "rg_eq_logm1")) {
			option_rango = RANGO_EQ_LOGM1;
		} else if (!strcmp(argv[i], "rg_eq_halflog")) {
			option_rango = RANGO_EQ_HALFLOG;
		} else if (!strcmp(argv[i], "rg_cte")) {
			option_rango = RANGO_CTE;
		} else if (argv[i][0] == 'i') {
			TIMES = atoi(&argv[i][1]);
		}
	}

	uint n;
	uchar *s;
	uint *r, *p, *h;
	fasta_nsrph(fn, &n, &s, &r, &p, &h);

	uint t;
	forn (t, TIMES) {
		uint *height = p;
		height_array(h, n, height);
		uint i, j;
		lowcompl(n, s, r, h, height);

		if (t != TIMES - 1) {
			recalc_nsrph(n, s, r, p, h);
		}
	}

	s[n - 1] = '\n';
	fwrite(s, 1, n, stdout);

	return 0;
}

