
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "bwt.h"
#include "macros.h"
#include "comprsa.h"

#define Printf(...) 	printf(__VA_ARGS__)
#if 0
#define Printf(...)
#endif

#include "tiempos.h"

#define randint(N) 	(uint)(((float)rand() / (float)(RAND_MAX - 1)) * (N))
void permutar(uint *arr, uint n) {
	uint i, r, t;
	forn (i, n) {
		r = randint(n - i);
		t = arr[i];
		arr[i] = arr[r];
		arr[r] = t;
	}
}

/* Warning: redefining constants from the implementation here */
#define __MAX_UINT 0xffffffff
#define ALPHA_PAD ((uchar)255)
#define sa_pnkmo(sa)	((sa)->n_k - 1)
uint sa_compressed_psi(sa_compressed *sa, uint i);
uint sa_inv_compressed_psi(uchar *s, sa_compressed *sa, uint ri, uint i);
uint sa_psi(sa_compressed *sa, uint i);
#define sa_half_lookup(sa, j) \
	((sa)->half_r ?  (sa)->half_r[(j)] : sa_lookup((sa)->sa_next, (j)))

#define TIME 1
#define MEM  2

void _test_bwt_benchmark(uchar *s, uint measure) {
	tiempo prebwt, posbwt,
		prer, posr, prep, posp,
		precompress, poscompress,
		precompr, poscompr, precompp, poscompp;

	srand(31416);
	uint i;
	uint n = strlen((char *)s);
	s[n] = ALPHA_PAD;
	++n;
	uint *p = pz_malloc(sizeof(uint) * n);
	uint *r = pz_malloc(sizeof(uint) * n);
	memcpy(p, s, n);

	uint *indices = NULL;
	if (measure & TIME) {
		indices = malloc(sizeof(uint) * n); // don't use pz_malloc
		forn (i, n) indices[i] = i;
		permutar(indices, n);
	}

	fprintf(stderr, "META PREBWT\n");
	getTickTime(&prebwt);
	bwt(NULL, p, r, NULL, n, NULL);
	forn(i, n) p[r[i]] = i;
	getTickTime(&posbwt);
	fprintf(stderr, "META POSBWT\n");

	if (measure & TIME) {
		uint x = 0;
		fprintf(stderr, "META PRER\n");
		getTickTime(&prer);
		forn (i, n) {
			x += r[indices[i]];
		}
		getTickTime(&posr);
		fprintf(stderr, "META POSR\n");

		fprintf(stderr, "META PREP\n");
		getTickTime(&prep);
		forn (i, n) {
			x += p[indices[i]];
		}
		getTickTime(&posp);
		fprintf(stderr, "META POSP\n");
		printf("%i\n", x);
	}

	pz_free(p);

	fprintf(stderr, "META PRECOMPRESS\n");
	getTickTime(&precompress);
	sa_compressed *sa = sa_compress(s, r, n); /* frees r */
	getTickTime(&poscompress);
	fprintf(stderr, "META POSCOMPRESS\n");

	if (measure & MEM) {
		fprintf(stderr, "META PRECOMPRESSED\n");
		fprintf(stderr, "META POSCOMPRESSED\n");
	}

	if (measure & TIME) {
		uint x = 0;
		fprintf(stderr, "META PRECOMPR\n");
		getTickTime(&precompr);
		forn (i, n) {
			x += sa_lookup(sa, indices[i]);
		}
		getTickTime(&poscompr);
		fprintf(stderr, "META POSCOMPR\n");

		fprintf(stderr, "META PRECOMPP\n");
		getTickTime(&precompp);
		forn (i, n) {
			x += sa_inv_lookup(s, sa, i);
		}
		getTickTime(&poscompp);
		fprintf(stderr, "META POSCOMPP\n");
		printf("%i\n", x);
		free(indices); // don't use pz_free

		fprintf(stderr, "TIME BWT %6.0f us\n", getTimeDiff(prebwt, posbwt) * 1000.0);
		fprintf(stderr, "TIME COMPRESS %6.0f us\n", getTimeDiff(precompress, poscompress) * 1000.0);
		fprintf(stderr, "TIME R %6.0f us\n", getTimeDiff(prer, posr) * 1000.0);
		fprintf(stderr, "TIME P %6.0f us\n", getTimeDiff(prep, posp) * 1000.0);
		fprintf(stderr, "TIME COMPR %6.0f us\n", getTimeDiff(precompr, poscompr) * 1000.0);
		fprintf(stderr, "TIME COMPP %6.0f us\n", getTimeDiff(precompp, poscompp) * 1000.0);
	}

	sa_destroy(sa);
}


typedef uint (*datafunc)(uint);

/* Calculate the index of the corresponding major/minor block
 * given a bit position */
#define maj_index(rks, pos) ((pos) / (rks)->rank.block_size / (rks)->rank.accmaj_span)
#define min_index(rks, pos) ((pos) / (rks)->rank.block_size)

/* _testpad in [0..31]
 * _datafunc :: uint -> uint
 */

#define Printable(X) 	(((X) < 'A' || (X) > 'Z') ? '$' : (X))
void _test_bwt(uchar *src) {
	uint slen = strlen((char *)src);
	uchar *s = pz_malloc(slen + 1);
	uint n;

	for (n = 5; n <= slen; ++n) {			/* All prefixes ascending } */
	//for (n = slen + 1; n > 4; --n) {		/* All prefixes descending } */
	//for (n = slen + 1; n == slen + 1; --n) {	/* Just src */
		strncpy((char *)s, (char *)src, n - 1);
		s[n - 1] = ALPHA_PAD;
		uint *p = pz_malloc(sizeof(uint) * n);
		uint *r = pz_malloc(sizeof(uint) * n);
		memcpy(p, s, n);
		uint i;
		fprintf(stderr, "Calculating BWT...\n");
		bwt(NULL, p, r, NULL, n, NULL);
		forn(i, n) p[r[i]] = i;

		Printf("------------------------\n");

		Printf("i:\t   ");
		forn (i, n) {
			Printf(" %.2u", i);
		}
		Printf("\n");

		Printf("s:\t   ");
		forn (i, n) {
			Printf("  %c", Printable(s[i]));
		}
		Printf("\n");

		Printf("SA:\t   ");
		forn (i, n) {
			Printf(" %.2u", r[i]);
		}
		Printf("\n");

#if 0
		Printf("iSA:\t   ");
		forn (i, n) {
			Printf(" %.2u", p[i]);
		}
		Printf("\n");
#endif

		Printf("bwt:\t   ");
		forn (i, n) {
			Printf("  %c", Printable(s[(r[i] == 0 ? n : r[i]) - 1]));
		}
		Printf("\n");

		/* r es el SUFFIX ARRAY */

		fprintf(stderr, "Compressing...\n");

		uint *r_destroy = pz_malloc(sizeof(uint) * n);
		forn(i, n) r_destroy[i] = r[i];
		sa_compressed *sa = sa_compress(s, r_destroy, n); /* frees r_destroy */

		fprintf(stderr, "Checking...\n");
		Printf("B:\t   ");
		forn (i, n) {
			Printf("  %u", ranksel_get(sa->rsb, i));
		}
		Printf("\n");

		Printf("rank:\t  0");
		forn (i, n) {
			Printf(" %2llu", ranksel_rank1(sa->rsb, i + 1));
		}
		Printf("\n");

#if __DEBUG_COMPSA
		Printf("psi:\t   ");
		forn (i, n) {
			Printf(" %.2u", sa->psi[i]);
		}
		Printf("\n");
#endif

		Printf("------------------------\n");

		Printf("LONGITUD = %u\n", n);
		//Printf("hola\n");
		if (sa->pad) {
			Printf("PAD! pnkmo = %u\n", sa_pnkmo(sa));
		}
		//Printf("chau\n");


#if __DEBUG_COMPSA
		Printf("CPSI:\t   ");
		forn (i, n / 2) {
			Printf(" %.2u", sa->compressed_psi[i]);
		}
		Printf("\n");
#endif

#if 0
			/// HALF
			Printf("HALF CPSI:\t   ");
			forn (i, sa->sa_next->n_k / 2 + sa->sa_next->pad) {
				Printf(" %.2u", sa->sa_next->compressed_psi[i]);
			}
			Printf("\n");
#endif

		Printf("PSI2:\t   ");
		forn (i, n) {
			uint psi2_i = sa_psi(sa, i);
			Printf(" %.2u", psi2_i);
#if __DEBUG_COMPSA
			assert(psi2_i == sa->psi[i]);
#endif
			psi2_i = psi2_i;
		}
		Printf("\n");


		Printf("SA2:\t   ");
		forn (i, n) {
			uint sa2_i = sa_lookup(sa, i);
			Printf(" %.2u", sa2_i);
			assert(sa2_i == r[i]);
		}
		Printf("\n");
		
		Printf("halfR:\t   ");
		forn (i, sa->n_k / 2 + sa->pad) {
			uint sa2_i = sa_half_lookup(sa, i);
			Printf(" %.2u", sa2_i);
		}
		Printf("\n");

		forn (i, sa->n_k / 2) {
			uint cpsii = sa_compressed_psi(sa, i);
			uint ri = r[cpsii] - 1;
			uint invcpsii = sa_inv_compressed_psi(s, sa, ri, cpsii);
			assert(invcpsii == i);
		}

		Printf("SAinv:\n");
		uint ri;
		forn (ri, sa->n_k) {
			printf("~~~\n");
			printf("~~~r(i) = %i\n", ri);
			printf("~~~p(r(i)) = %i\n", p[ri]);
			printf("~~~longitud = %i\n", n);
			uint sainv = sa_inv_lookup(s, sa, ri);
			assert(sainv == p[ri]);
		}
#if 0
#endif
	
		sa_destroy(sa);
		pz_free(p);
		pz_free(r);
	}
	pz_free(s);
}

void _test_ranksel(uint _testpad, datafunc _datafunc, uint mini, uint maxi) {
	uint size32;
	for (size32 = mini; size32 < maxi; ++size32) {
		uint size_bits = 32 * size32 - _testpad;
		uint i, j, pos;

		fprintf(stderr, "SIZE: %i\n", size32);

		word_type *data = pz_malloc(sizeof(word_type) * size32);
		for (i = 0; i < size32; ++i) {
			if (i == size32 - 1) {
				//data[i] = (*_datafunc)(i) & ((1 << _testpad) - 1);
				data[i] = 0;
			} else {
				data[i] = (*_datafunc)(i);
			}
		}

		ranksel *RS = ranksel_create(size_bits, data);
		uint rank1_actual = 0;
		uint rank0_actual = 0;
		uint select1_actual = 0;
		uint select0_actual = 0;
		uint pmin = 0;

		/*
		 * accum_min = sum_{k = 0}^{(pos - 1) div block_size }{ data_{k} }
		 * accum_maj = sum_{k = 0}^{((pos - 1) mod block_size) div accmaj_span}{ data_{k} }
		 */

		pos = 0;


		for (i = 0; i < size32; ++i) {
			for (j = 0; j < 32; ++j) {
				uint uno = data[i] & (1 << j);
				if (uno) {
					select1_actual = pos;
					++rank1_actual;
					++pmin;
				} else {
					select0_actual = pos;
					++rank0_actual;
				}

				++pos;

				if (pos == size32 * 32 - _testpad) break;

				uint rcalc1 = ranksel_rank1(RS, pos);
				assert(rcalc1 == rank1_actual);

				uint rcalc0 = ranksel_rank0(RS, pos);
				assert(rcalc0 == rank0_actual);

				if (rank1_actual > 0) {
					uint scalc1 = ranksel_select1(RS, rank1_actual);
					assert(scalc1 == select1_actual);
				}

				if (rank0_actual > 0) {
					uint scalc0 = ranksel_select0(RS, rank0_actual);
					assert(scalc0 == select0_actual);
				}
			}
		}


		uint rcalc1 = ranksel_rank1(RS, pos);
		assert(rcalc1 == rank1_actual);

		uint rcalc0 = ranksel_rank0(RS, pos);
		assert(rcalc0 == rank0_actual);

		if (rank1_actual > 0) {
			uint scalc1 = ranksel_select1(RS, rank1_actual);
			assert(scalc1 == select1_actual);
		}

		if (rank0_actual > 0) {
			uint scalc0 = ranksel_select0(RS, rank0_actual);
			assert(scalc0 == select0_actual);
		}

		ranksel_destroy(RS);
		pz_free(data);
	}
}

uint func_ceros(uint i) { i++; return 0; }
uint func_unos(uint i) { i++; return __MAX_UINT; }
uint func_bit(uint i) {
	return 1 << (i % 32);
}
uint func_cobit(uint i) {
	return ~(1 << (i % 32));
}
uint func_loca(uint i) {
	return ((0xf0ff - i) << 16) + i;
}
uint func_otra(uint i) {
	return (i * i * i * i) ^ ~((i * i * i) << 8) ^ ((i * i) << 16) ^ ~(i << 24);
}

void _test_algo(char *src, char *algo) {
	/*
	if (!strcmp(algo, "bits") || !strcmp(algo, "all")) {
		printf("testing bits");
		_test_bits(100);
		_test_trie();
	}
	*/
	if (!strcmp(algo, "rs") || !strcmp(algo, "all")) {
#define DOTEST(F) \
	for (uint pad = 0; pad < MAXP; ++pad) { \
		printf("%s\n", #F); \
		printf("pad=%i\n", pad); \
		_test_ranksel(pad, F, MINI, MAXI); \
	}

#define MINI 1
#define MAXI 100
#define MAXP 28
	DOTEST(func_otra);
	DOTEST(func_ceros);
	DOTEST(func_unos);
	DOTEST(func_bit);
	DOTEST(func_cobit);
	DOTEST(func_loca);
#undef MINI
#undef MAXI
#undef MAXP
#define MINI 1
#define MAXI 500
#define MAXP 1
	DOTEST(func_otra);
	DOTEST(func_ceros);
	DOTEST(func_unos);
	DOTEST(func_bit);
	DOTEST(func_cobit);
	DOTEST(func_loca);
#undef MINI
#undef MAXI
#undef MAXP
#undef DOTEST
	}

	if (!strcmp(algo, "sa") || !strcmp(algo, "all")) {
		if (src == NULL) {
			src = "ACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTA"
				"TTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAA"
				"ATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTAT"
				"TCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAA"
				"TGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATT"
				"CAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAAT"
				"GAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTC"
				"AAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATG"
				"AGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCA"
				"AGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGA"
				"GACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAA"
				"GTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAG"
				"ACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAG"
				"TAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGA"
				"CACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGT"
				"AGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGAC"
				"ACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTA"
				"GGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACA"
				"CATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAG"
				"GGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACAC"
				"ATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGG"
				"GTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACA"
				"TACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGG"
				"TAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACAT"
				"ACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGT"
				"AGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATA"
				"CCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTA"
				"GTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATAC"
				"CTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAG"
				"TTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACC"
				"TATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGT"
				"TATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCT"
				"ATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTT"
				"ATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTA"
				"TACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTA"
				"TTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTAT"
				"ACATTATTCAAGTAGGGTAGTTATTAAAAAATGAGACACATACCTATACATTATTCAAGTAGGGTAGTTAT"
				"TAAAAAATGAGACACAT";
		}
		_test_bwt((uchar *)src);
	}
}

int main(int argc, char **argv) {

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <option> <param>\n", argv[0]);
		fprintf(stderr, "Options:\n"
		                "  -mem  <file>    process <file> and measure memory\n"
		        	"  -time <file>    process <file> and measure time\n"
		        	"  -test <algo>    test the algorithms <algo> in {rs, sa, all}\n"
		        	"  -filesa <file>  test compressed suffix array for the file\n"
		);
		exit(1);
	}

	int otest = 0;
	uint measure = 0;
	if (!strcmp(argv[1], "-mem")) measure = MEM;
	else if (!strcmp(argv[1], "-time")) measure = TIME;
	else if (!strcmp(argv[1], "-test")) otest = 1;
	else if (!strcmp(argv[1], "-filesa")) otest = 2;
	else {
		fprintf(stderr, "Unknown option.\n");
		exit(1);
	}

	char *src = NULL;
	if (otest == 0 || otest == 2) {
		FILE *f = fopen(argv[2], "r");
		if (!f) {
			fprintf(stderr, "Cannot open %s\n", argv[2]);
			exit(1);
		}
		uint len = filesize(argv[2]);
		src = pz_malloc(len + 1);
		fprintf(stderr, "Reading file...\n");
		if (!fread(src, sizeof(char), len, f)) {
			fprintf(stderr, "Cannot read %s\n", argv[2]);
			fclose(f);
			exit(1);
		}
		fclose(f);
		src[len] = '\0';
	}
	if (otest == 0) {
		_test_bwt_benchmark((uchar *)src, measure);
	} else if (otest == 1) {
		_test_algo(NULL, argv[2]);
	} else if (otest == 2) {
		_test_algo(src, "sa");
	}
	
	if (otest == 0 || otest == 2) {
		pz_free(src);
	}
	return 0;
}

