#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "tipos.h"
#include "common.h"

#include "macros.h"

#define db_file "db-srv.txt"

typedef struct {
	int rev;
	int n, zero_mtf, mtf_entropy;
	int filename_a, filename_b;
	uint64 time_bwt;
	uint64 lpat_size, lpat3_size, lpat3dl0_size;
	double kinv_lcp;
	char hostname;
} run_entrie;

double re_K(const run_entrie* this) {
	return  this->rev?(this->n - this->zero_mtf):0;
}

static uchar *mfiles;
static char** files;
static const char** labels;
static uint nfiles;

static const char* get_filename(const char* fn) {
	const char* p = fn, *lp;
	if (!fn) return fn;
	while ((lp = strchr(p, '/'))) p = lp+1;
	return p;
}

void load_files(const char* filename) {
	uchar *p, lv;
	uint i, j, len;
	nfiles = 0;

	p = mfiles = loadStrFile(filename, &len);
	if (!mfiles) exit(0);
	lv = 0;
	forn(i, len) {
		if (*p == '\n') *p = 0;
		if (!lv && *p) nfiles++;
		lv = *p; p++;
	}
	files = pz_malloc(nfiles*sizeof(char*));
	labels = pz_malloc(nfiles*sizeof(char*));
	p = mfiles;
	lv = 0; j = 0;
	forn(i, len) {
		if (!lv && *p) {
			files[j] = (char*)p;
			labels[j] = get_filename((char *)p);
			++j;
		}
		lv = *p; p++;
	}
	fprintf(stderr, "%d files loaded.\n",  nfiles);
}

void load_labels(const char* filename) {
	uchar *p, lv;
	uint i, j, len;
	uint nlabels = 0;

	p = mfiles = loadStrFile(filename, &len);
	if (!mfiles) exit(0);
	lv = 0;
	forn(i, len) {
		if (*p == '\n') *p = 0;
		if (!lv && *p) nlabels++;
		lv = *p; p++;
	}
	if (nlabels != nfiles) {
		fprintf(stderr, "Error: number of files and number of labels don't match.\n");
		exit(1);
	}
	p = mfiles;
	lv = 0; j = 0;
	forn(i, len) {
		if (!lv && *p) {
			labels[j] = (char *)p;
			++j;
		}
		lv = *p; p++;
	}
}

void unload_files(void) {
	pz_free(mfiles);
	pz_free(files);
	pz_free(labels);
}

int file_id(const char* file) {
	uint i;
	if (!file[0]) return -1;
	forn(i, nfiles) if (strstr(files[i], file)) return i;
	return -1;
}

char prim(const char* str) { return *str; }

#define read_field(field, func) if (!strncmp((char*)head, #field"\t", strlen(#field)+1) || !strcmp((char*)head, #field)) this->field = func((char*)(p));
uchar* parse_line(run_entrie* this, uchar* p, uchar* head) {
	uchar *q;

	bool ok = TRUE;
	while(ok) {
		q = p; while(*q && *q != '\t' && *q != '\n') q++; if (*q == '\t') *(q++) = 0; else if (*q == '\n' || !*q) ok = *(q++) = 0;

		read_field(rev, atoi)
		read_field(n, atoi)
		read_field(zero_mtf, atoi)
		read_field(filename_a, file_id)
		read_field(filename_b, file_id)
		read_field(hostname, prim)
		read_field(time_bwt, atol)
		read_field(mtf_entropy, atoi)
		read_field(lpat_size, atol)
		read_field(lpat3_size, atol)
		read_field(lpat3dl0_size, atol)
		read_field(kinv_lcp, atof)

		p = q;
		while(*head && *head != '\t') head++; if (*head == '\t') head++;
	}
	return p;
}

run_entrie *mre;
#define mval1(i) mre[nfiles*nfiles+(i)]
#define mval2(i,j) mre[nfiles*(j)+(i)]

#define SWAP(X, Y) { X ^= Y; Y ^= X; X ^= Y; }

#define NOTCALC -1.0

double calc_dist(int ida, int idb) {
	uint64 kx, ky;
	if (!mval1(ida).rev || !mval1(idb).rev || !mval2(ida, idb).rev) return NOTCALC;
	kx = re_K(&mval1(ida));
	ky = re_K(&mval1(idb));
	if (kx < ky) SWAP(kx, ky);
	return (double)(re_K(&mval2(ida, idb)) - ky) / (double)kx;
}

double calc_znorm(int ida, int idb) {
	if (!mval1(ida).rev || !mval1(idb).rev || !mval2(ida, idb).rev) return NOTCALC;
	if (ida==idb) return 1.0 - (double)(mval1(ida).zero_mtf) / (double)(mval1(ida).n);
	double res = (double)(mval2(ida, idb).zero_mtf - mval1(ida).zero_mtf - mval1(idb).zero_mtf) / (double)( mval1(ida).n + mval1(idb).n );
	if (res < 0.0) res = 0.0;
	return 1.0 - res;
}

double calc_zdif(int ida, int idb) {
	if (!mval1(ida).rev || !mval1(idb).rev || !mval2(ida, idb).rev) return NOTCALC;
	if (ida==idb) return mval1(ida).zero_mtf;
	double res = (double)(mval2(ida, idb).zero_mtf - mval1(ida).zero_mtf - mval1(idb).zero_mtf);
	if (res < 0.0) res = 0.0;
	return res;
}

double calc_dent(int ida, int idb) {
	uint64 kx, ky;
	if (!mval1(ida).rev || !mval1(idb).rev || !mval2(ida, idb).rev) return NOTCALC;
	kx = mval1(ida).mtf_entropy;
	ky = mval1(idb).mtf_entropy;
	if (kx < ky) SWAP(kx, ky);
	return (double)(mval2(ida, idb).mtf_entropy - ky) / (double)kx;
}

double calc_pat(int ida, int idb) {
	uint64 kx, ky;
	if (!mval1(ida).rev || !mval1(idb).rev || !mval2(ida, idb).rev) return NOTCALC;
	kx = mval1(ida).lpat_size;
	ky = mval1(idb).lpat_size;
	if (kx < ky) SWAP(kx, ky);
	return (double)(mval2(ida, idb).lpat_size - ky) / (double)kx;
}

double calc_pat3(int ida, int idb) {
	uint64 kx, ky;
	if (!mval1(ida).rev || !mval1(idb).rev || !mval2(ida, idb).rev) return NOTCALC;
	kx = mval1(ida).lpat3_size;
	ky = mval1(idb).lpat3_size;
	if (kx < ky) SWAP(kx, ky);
	return (double)(mval2(ida, idb).lpat3_size - ky) / (double)kx;
}

double calc_pat3dl0(int ida, int idb) {
	uint64 kx, ky;
	if (!mval1(ida).rev || !mval1(idb).rev || !mval2(ida, idb).rev) return NOTCALC;
	kx = mval1(ida).lpat3dl0_size;
	ky = mval1(idb).lpat3dl0_size;
	if (kx < ky) SWAP(kx, ky);
	return (double)(mval2(ida, idb).lpat3dl0_size - ky) / (double)kx;
}

double calc_klcp(int ida, int idb) {
	long double kx, ky;
	if (!mval1(ida).rev || !mval1(idb).rev || !mval2(ida, idb).rev) return NOTCALC;
	kx = mval1(ida).kinv_lcp;
	ky = mval1(idb).kinv_lcp;
	if (kx < ky) { long double k_ = kx; kx = ky; ky = k_; }
	return (double)(mval2(ida, idb).kinv_lcp - ky) / (double)kx;
}


static const char* db_header = "rev\tfilename_a\tfilename_b";
void load_db(const char* dbfn) {
	uint l;
	uchar *p, *q;
	uint rd = 0;
	int ida, idb;
	run_entrie re;

	mre = pz_malloc(nfiles*(nfiles+1)*sizeof(run_entrie));
	memset(mre, 0, nfiles*(nfiles+1)*sizeof(run_entrie));

	p = loadStrFile(dbfn, &l);
	if (p) {
		if (strncmp(db_header, (char*)p, strlen(db_header)) != 0) {
			fprintf(stderr, "Invalid DB file\n");
		} else {
			p[l-1] = 0;
			q = p;
#			define char_skip(q, ch) { while (*q && *q != ch) q++; if (*q) *(q++) = 0; }
			/* Ignora los headers */
			char_skip(q, '\n')
			while (*q) {
				re.filename_a = re.filename_b = -1;
				q = parse_line(&re , q, p);
				ida = re.filename_a;
				idb = re.filename_b;
				if (re.rev <= 0) re.rev = 1; // Hack for unknown revs

				if (idb == -1) { // Single
					if (ida != -1) {
						rd += !mval1(ida).rev;
						if (re.rev > mval1(ida).rev) mval1(ida) = re;
					}
				} else {
					if (ida != -1) {
						rd += !mval2(ida, idb).rev;
						mval2(ida, idb) = mval2(idb, ida) = re;
					}
				}
			}
			fprintf(stderr, "%u entries loaded, %u remaining.\n", rd, nfiles*(nfiles+3)/2 - rd);
		}
		pz_free(p);
	}
}

void unload_db(void) {
	pz_free(mre);
}

void show_stats() {
	uint i, j;
	uint done[256];
	forn(i, 256) done[i] = 0;

	/** SHOW **/
	fprintf(stderr, "singl: ");
	forn(i, nfiles) { fprintf(stderr, "%c", ".*"[mval1(i).rev!=0]); done[(int)(mval1(i).hostname)]++; } fprintf(stderr, "\n");

	forn(i, nfiles) {
		fprintf(stderr, " %4d  ", i);
		forn(j, i+1) { fprintf(stderr, "%c", mval2(i,j).rev?mval2(i,j).hostname:'.'); done[(int)mval2(i,j).hostname]++; }
		fprintf(stderr, " <-- %s", get_filename(files[i]));
		fprintf(stderr, "\n");
	}

	forn(i, 256) if (i && done[i]) fprintf(stderr, "%c -> %u\n", i, done[i]);
}

typedef double func_dist(int, int);
void save_cuenta(const char* fn, func_dist fun) {
	uint i, j;
	FILE *f;

	if (!(f= fopen(fn, "wt"))) { perror("creating dists file"); return; }

	// Headers
	forn(i, nfiles) fprintf(f, "\t%s", labels[i]); fprintf(f, "\n");
	// Tabla
	forn(i, nfiles) {
		fprintf(f, "%s", labels[i]);
		forn(j, nfiles) fprintf(f, "\t%.12f", fun(i, j));
		fprintf(f, "\n");
	}
	fclose(f);
	return;
}

void save_infodb(const char* fn) {
	uint i, j;
	FILE *f;

	if (!(f= fopen(fn, "wt"))) { perror("creating dists file"); return; }

	fprintf(f, "rev\tfile_a\tfile_b\tn_a\tn_b\tk_a\tk_b\tn_ab\tk_ab\tdist_ab\n");

	forn(i, nfiles) forn(j, nfiles) if (mval2(i,j).rev) if (!strncmp(get_filename(files[i]), get_filename(files[j]), 6)) {
		run_entrie *fa=&mval1(i), *fb=&mval1(j);
		fprintf(f, "%d\t%s\t%s\t%d\t%d\t%f\t%f\t%d\t%f\t%.12f\n",
			mval2(i,j).rev, get_filename(files[i]), get_filename(files[j]),
			fa->rev?fa->n:-1, fb->rev?fb->n:-1, re_K(fa), re_K(fb),
			mval2(i,j).n, re_K(&mval2(i,j)), calc_dist(i, j));
	}

	fclose(f);
}

void save_dbsingle(const char* fn) {
	uint i; FILE *f;
	char esp[32], *p;
	if (!(f= fopen(fn, "wt"))) { perror("creating dbsingle file"); return; }

	fprintf(f, "rev\tfile\tespecie\tn\tk\tzero_mtf\n");

	forn(i, nfiles) if (mval1(i).rev) {
		strncpy(esp, get_filename(files[i]), 32);
		for(p=esp; *p; p++) if (*p=='.') *p=0;

		fprintf(f, "%d\t%s\t%s\t%d\t%.0f\t%d\n",
			mval1(i).rev, get_filename(files[i]), esp,
			mval1(i).n, re_K(&mval1(i)), mval1(i).zero_mtf);
	}

	fclose(f);
}

int main(int argc, char* argv[]) {
	/* Parsea los parámetros */
	if (argc != 3 && argc != 4) {
		fprintf(stderr, "Use: %s <file_list> <db_file> [<label-file>]\n", argv[0]);
		return 1;
	}
	load_files(argv[1]);
	load_db(argv[2]);
	if (argc == 4) {
		load_labels(argv[3]);
	}

	show_stats();
	save_cuenta("mat-dist.txt", calc_dist);
	save_cuenta("mat-znorm.txt", calc_znorm);
	save_cuenta("mat-zdif.txt", calc_zdif);
	save_cuenta("mat-dent.txt", calc_dent);
	save_cuenta("mat-pat.txt", calc_pat);
	save_cuenta("mat-pat3.txt", calc_pat3);
	save_cuenta("mat-pat3dl0.txt", calc_pat3dl0);
	save_cuenta("mat-klcp.txt", calc_klcp);
	save_infodb("db-info.txt");
	save_dbsingle("db-singles.txt");

	unload_files();
	unload_db();

	return 0;
}


