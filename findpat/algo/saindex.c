#include "saindex.h"
#include "bitarray.h"
#include "sais.h"
#include "lcp.h"
#include "sa_search.h"

#include "macros.h"

#define MAX_LOOKUP 16

int saindex_init(sa_index* this, const uchar* s, uint n, uint lookup, uint flags) {
	uint ndna, nlkp, i, j, pad;
	bitarray vl, *buf;

	// Maximun lookup size
	if (lookup > MAX_LOOKUP) lookup = MAX_LOOKUP;
	if ((flags & SA_DNA_ALPHA) == 0) lookup = 0;
	
	this->n = n;
	this->flags = flags;
	this->lookupsz = lookup;

	this->s = NULL;
	this->r = NULL;
	this->p = NULL;
	
	this->llcp = NULL;
	this->rlcp = NULL;
	this->lk = NULL;
	
	this->r = pz_malloc(sizeof(uint) * this->n);
	if (!this->r) return -1;

	/* Builds the suffix array */
	sais(s, this->r, n);

	/* Compute the LCP value */
	this->h = pz_malloc(sizeof(uchar) * this->n);
	lcp_mem3(this->n, s, this->r, (uchar*)this->h);
	((uchar*)this->h)[n-1] = 0;

	/* Compute lookup table L[] */
	if (this->lookupsz && (flags & SA_DNA_ALPHA)) {
		nlkp = 1ULL << (uint64)(this->lookupsz*2);
		this->lk = pz_malloc(sizeof(uint) * nlkp);
		// bitarray patt = 0, msk = nlkp - 1U;
		const uchar alph[4] = "ACGT";
		const uchar* h = this->h;
		uint eq;
		const uint *r = this->r;
		j = 0;
		forn(i, nlkp) { // lk[i] marks the right border  of the "i" bucket [)
			eq = 0;
			while(eq < lookup && j < n) {
				if (r[j]+eq >= n || s[r[j]+eq] < alph[(i >> ((lookup-1-eq)*2)) & 3]) { if (h[j] < eq) eq = h[j]; j++; } // Caso s[r[j]] < i
				else if (s[r[j]+eq] > alph[(i >> ((lookup-1-eq)*2)) & 3]) break; // Caso s[r[j]] > i
				else eq++;
			}
			if (eq == lookup) {
				while (j < n && h[j] >= lookup) j++;
				if (j<n) j++;
			}
			this->lk[i] = j;
		}
	}

	/* Copy the source to the s array */
	if (flags & SA_NO_SRC) {
		this->s = NULL;
	} else
	if (flags & SA_DNA_ALPHA) {
		ndna = _bits_to_bytes(_pad_to_wordsize(2ULL*this->n, ba_word_size));
		this->s = pz_malloc(ndna);
		if (!this->s) return -1;
		buf = (bitarray*)this->s;
		pad = 0; vl = 0;
		const uchar* ss = s;
		dforn(i, this->n) {
			switch (*ss) {
				case 'a': case 'A': break;
				case 'c': case 'C': vl |= 1 << pad; break;
				case 'g': case 'G': vl |= 2 << pad; break;
				case 't': case 'T': vl |= 3 << pad; break;
				default: break;
			}
			pad += 2;
			if (pad == ba_word_size) { pad = 0; *buf = vl; vl = 0; buf++; }
			ss++;
		}
		if (pad) *buf = vl;
	} else {
		if (flags & SA_STORE_S) {
			this->s = pz_malloc(sizeof(uchar) * this->n);
			memcpy(this->s, s, this->n);
		} else {
			this->s = s;
		}
	}

	/* Build Llcp / Rlcp */
	if ((flags & SA_NO_LRLCP) == 0) {
		const uchar* h = this->h;
		if ((flags & SA_STORE_LCP) == 0) {
			this->rlcp = this->h; this->h = NULL;
		} else {
			this->rlcp = pz_malloc(sizeof(uchar) * this->n);
		}
		this->llcp = pz_malloc(sizeof(uchar) * this->n);
		build_lrlcp_lk_8(n, h, this->llcp, this->rlcp, lookup, this->lk);
	}

	if ((flags & SA_STORE_LCP) == 0 && this->h) {
		pz_free(this->h); this->h = NULL;
	}

	/* Build P */
	if (flags & SA_STORE_P) {
		this->p = pz_malloc(sizeof(uint) * this->n);
		if (!this->p) return -1;
		uint *p = this->p;
		uint *r = this->r;
		dforn(i, this->n) p[r[i]] = i;
	}

	return 0;
}

/* Destroy and free the memory of <sa>. */
void saindex_destroy(sa_index *this) {
	#define _pz_clear(X) if(X) { pz_free(X); X = NULL; }
	if ((this->flags & SA_DNA_ALPHA) || (this->flags & SA_STORE_S)) _pz_clear(this->s);
	_pz_clear(this->r);
	_pz_clear(this->p);
	_pz_clear(this->h);
	_pz_clear(this->llcp);
	_pz_clear(this->rlcp);
	_pz_clear(this->lk);
	this->n = 0;
}

/* Loads/stores the structure (and substructures) to/from a file. */
int saindex_load(sa_index* this, FILE * fp) {
	int ret = 0;
	uint ndna, nlkp;
	
	this->s = NULL;
	this->r = NULL;
	this->p = NULL;
	
	this->llcp = NULL;
	this->rlcp = NULL;
	this->lk = NULL;
	
	ret -= fread(&this->flags, sizeof(this->flags), 1, fp) != 1; // flags
	ret -= fread(&this->lookupsz, sizeof(this->lookupsz), 1, fp) != 1; // lookupsz
	ret -= fread(&this->n, sizeof(this->n), 1, fp) != 1; // n
	if (ret) return ret;

	/* Load R[] */
	this->r = pz_malloc(sizeof(uint) * this->n);
	if (!this->r) return -1;
	ret -= fread(this->r, sizeof(uint), this->n, fp) != this->n;
	
	if ((this->flags & SA_NO_LRLCP) == 0) {
		/* Load Llcp and Rlcp */
		this->llcp = pz_malloc(sizeof(uchar) * this->n);
		if (!this->llcp) return -1;
		ret -= fread(this->llcp, sizeof(uchar), this->n, fp) != this->n;
	
		this->rlcp = pz_malloc(sizeof(uchar) * this->n);
		if (!this->rlcp) return -1;
		ret -= fread(this->rlcp, sizeof(uchar), this->n, fp) != this->n;
	}
	if (ret) return ret;
	
	/* Load S[] */
	if (this->flags & SA_NO_SRC) {
		// Nothing to load
	} else
	if (this->flags & SA_DNA_ALPHA) {
		ndna = _bits_to_bytes(_pad_to_wordsize(2ULL*this->n, ba_word_size));
		this->s = pz_malloc(sizeof(uchar) * ndna);
		if (!this->s) return -1;
		ret -= fread(this->s, sizeof(uchar), ndna, fp) != ndna;
	} else {
		this->s = pz_malloc(sizeof(uchar) * this->n);
		if (!this->s) return -1;
		this->flags |= SA_STORE_S;
		ret -= fread(this->s, sizeof(uchar), this->n, fp) != this->n;
	}
	if (ret) return ret;

	/* Load L[] */
	if (this->lookupsz && (this->flags & SA_DNA_ALPHA)) {
		nlkp = 1ULL << (uint64)(this->lookupsz*2);
		this->lk = pz_malloc(sizeof(uint) * nlkp);
		if (!this->lk) return -1;
		ret -= fread(this->lk, sizeof(uint), nlkp, fp) != nlkp;
	}

	return ret;
}

int saindex_store(sa_index* this, FILE * fp) {
	int ret = 0;
	uint ndna, nlkp;

	/* Store header */
	ret -= fwrite(&this->flags, sizeof(this->flags), 1, fp) != 1; // flags
	ret -= fwrite(&this->lookupsz, sizeof(this->lookupsz), 1, fp) != 1; // lookupsz
	ret -= fwrite(&this->n, sizeof(this->n), 1, fp) != 1; // n
	if (ret) return ret;

	/* Store R[] */
	ret -= fwrite(this->r, sizeof(uint), this->n, fp) != this->n;
	
	if ((this->flags & SA_NO_LRLCP) == 0) {
		/* Store Llcp and Rlcp */
		ret -= fwrite(this->llcp, sizeof(uchar), this->n, fp) != this->n;
		ret -= fwrite(this->rlcp, sizeof(uchar), this->n, fp) != this->n;
	}
	if (ret) return ret;
	
	/* Store S[] */
	if (this->flags & SA_NO_SRC) {
		// Nothing to store
	} else
	if (this->flags & SA_DNA_ALPHA) {
		ndna = _bits_to_bytes(_pad_to_wordsize(2ULL*this->n, ba_word_size));
		ret -= fwrite(this->s, sizeof(uchar), ndna, fp) != ndna;
	} else {
		ret -= fwrite(this->s, sizeof(uchar), this->n, fp) != this->n;
	}
	if (ret) return ret;

	/* Store L[] */
	if (this->lookupsz && (this->flags & SA_DNA_ALPHA)) {
		nlkp = 1ULL << (uint64)(this->lookupsz*2);
		ret -= fwrite(this->lk, sizeof(uint), nlkp, fp) != nlkp;
	}
	
	return ret;
}

void saindex_show(const sa_index* this, FILE* fp) {
	uint i, j, nlkp, lv;
	/* Show main information */
	fprintf(fp, "SAIndex: n=%u flags=%x\n", this->n, this->flags);

	/* Show src */
	if (!this->s) {
		fprintf(fp, "src = NULL\n");
	} else {
		fprintf(fp, "src = «");
		if (this->flags & SA_DNA_ALPHA) {
			forn(i, this->n) fputc("ACGT"[bita_get(this->s, 2*i) + bita_get(this->s, 2*i+1)*2], fp);
		} else {
			fwrite(this->s, 1, this->n, fp);
		}
		fprintf(fp, "»\n");
	}

	/* Show Suffix Array */
	fprintf(fp, " i  ");
	if (this->llcp) fprintf(fp, "Lcp ");
	if (this->rlcp) fprintf(fp, "Rcp ");
	if (this->h) fprintf(fp, "lcp ");
	if (this->r) fprintf(fp, "R[] ");
	if (this->s) fprintf(fp, "S[R[i]..]");
	fprintf(fp, "\n");
	forn(i, this->n) {
		fprintf(fp, "%3u ", i);
		if (this->llcp) fprintf(fp, "%3u ", ((uchar*)this->llcp)[i]);
		if (this->rlcp) fprintf(fp, "%3u ", ((uchar*)this->rlcp)[i]);
		if (this->h) fprintf(fp, "%3u ", ((uchar*)this->h)[i]);
		if (this->r) fprintf(fp, "%3u ", this->r[i]);
		if (this->s) {
			if (this->flags & SA_DNA_ALPHA) {
				forsn(j, this->r[i], this->n) fputc("ACGT"[bita_get(this->s, 2*j) + bita_get(this->s, 2*j+1)*2], fp);
			} else {
				fwrite(this->s+this->r[i], 1, this->n-this->r[i], fp);
			}
		}
		fprintf(fp, "\n");
	}

	/* Show lookup table */
	if (this->lookupsz && (this->flags & SA_DNA_ALPHA)) {
		nlkp = 1ULL << (uint64)(this->lookupsz*2);
		char patt[32];
		patt[this->lookupsz] = 0;
		lv = 0;
		forn(i, nlkp) {
			forn(j, this->lookupsz) patt[this->lookupsz-1-j] = "ACGT"[(i >> (2*j)) & 3];
			fprintf(fp, "%s = [%3u, %3u)\n", patt, lv, this->lk[i]);
			lv = this->lk[i];
		}
	}
}

bool saindex_mmsearch(sa_index* this, uchar* pat, uint len, uint* from, uint* to) {
	if (this->flags & SA_DNA_ALPHA) {
		return sa_mm_search_dna_8(this->s, this->r, this->n, pat, len, from, to, this->llcp, this->rlcp, this->lookupsz, this->lk);
	} else {
		return sa_mm_search_8(this->s, this->r, this->n, pat, len, from, to, this->llcp, this->rlcp);
	}
}
