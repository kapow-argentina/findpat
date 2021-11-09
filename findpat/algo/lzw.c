#include <stdlib.h>
#include <string.h>
#include "lzw.h"

#include "macros.h"

/*** Patricia ***/

typedef struct str_patnode {
	uint prnt, chlds, code;
	uint dat[256];
	uchar* st; uint len;
} patnode;

inline void patnode_create(patnode* this, uint prnt, uchar* st, uint len) {
	this->prnt = prnt;
	this->code = LZW_DICTSIZE;
	this->st = st;
	this->len = len;
	this->chlds = 0;
	memset(this->dat, 0, sizeof(uint)*256);
}

inline void patnode_addchild(patnode* this, uchar ch, uint nd) {
	this->dat[ch] = nd;
	this->chlds++;
}

inline void patnode_delchild(patnode* this, uchar ch) {
	this->dat[ch] = 0;
	this->chlds--;
}

typedef struct {
	patnode* nodes;
	uint freez, *freend;
	uint root, size;
} patricia;

inline uint patricia_new_node(patricia* this) {
	return this->freend[--(this->freez)];
}
inline void patricia_delete_node(patricia* this, uint nd) {
	this->freend[this->freez++] = nd;
}

void patricia_create(patricia* this, uint size) {
	uint i;
	this->nodes = pz_malloc(2 * size * sizeof(patnode));
	this->freend = pz_malloc(2 * size * sizeof(uint));
	this->freez = 2*size-1;
	this->size = size;
	forn(i, 2*size-1) this->freend[i] = 2*size-1-i;
	/* patnode at position 0 is never used. We only need 2*codes-1 patnodes.
	 * A patnode index of 0 means "not used". */
	this->root = patricia_new_node(this);
	patnode_create(this->nodes+this->root, 0, NULL, 0);
}

void patricia_destroy(patricia* this) {
	pz_free(this->nodes);
	pz_free(this->freend);
}

uint64 patricia_mem_used(const patricia* this) {
	uint res = sizeof(patricia)
	  + sizeof(patnode) * 2 * this->size
	  + sizeof(uint) * 2 * this->size;
	return res;
}

uint64 patricia_mem_need(const patricia* this) {
	uint res = sizeof(patricia)
	  + sizeof(patnode) * (2 * this->size - this->freez)
	  + sizeof(uint) * 2 * this->size;
	return res;
}

uint patricia_invariant_rec(const patricia *this, uint nd) {
	uint i, ch = 0;
	forn(i, 256) if (this->nodes[nd].dat[i]) {
		ch++;
		if (nd != this->nodes[this->nodes[nd].dat[i]].prnt) return 0;
		if (!(this->nodes[this->nodes[nd].dat[i]].st) || (i != *(this->nodes[this->nodes[nd].dat[i]].st))) return 0;
		if (!patricia_invariant_rec(this, this->nodes[nd].dat[i])) return 0;
	}
	if (ch != this->nodes[nd].chlds) return 0;
	return 1;
}
uint patricia_invariant(const patricia *this) {
	return patricia_invariant_rec(this, this->root);
}

void patricia_rec_show(const patricia *this, uint nd, uint sps) {
	uint i, ch;
	fprintf(stderr, "(%4u)  ", nd);
	forn(i, sps) fprintf(stderr, " ");
	if (this->nodes[nd].st) if (!fwrite(this->nodes[nd].st, 1, this->nodes[nd].len, stderr)) perror("ERROR writing to stderr");
	if (this->nodes[nd].code != LZW_DICTSIZE) fprintf(stderr, " ==> %u", this->nodes[nd].code);
	fprintf(stderr, "\n");
	sps += this->nodes[nd].len;
	ch = 0;
	forn(i, 256) if (this->nodes[nd].dat[i]) {
		ch++;
		if (nd != this->nodes[this->nodes[nd].dat[i]].prnt) ppz_error("Invalid parent-child at position: %d", i);
		patricia_rec_show(this, this->nodes[nd].dat[i], sps);
		if (i != *(this->nodes[this->nodes[nd].dat[i]].st)) ppz_error("Invalid child at position: %d", i);
	}
	if (ch != this->nodes[nd].chlds) ppz_error("Invalid child count: %d", ch);
}

void patricia_show(patricia* this) {
	patricia_rec_show(this, this->root, 0);
}

uint patricia_insert_code(patricia* this, uchar* s, uint n, uint code) {
	uint *p = &(this->root);
	uint q = *p, lq = 0, l = 0, m = n;
	uchar *px = s, *st = 0;
	/* Consume prefijos */
	while (q && m) {
		l = this->nodes[q].len;
		st = this->nodes[q].st;
		while (m && l && *st == *s) { l--; m--; st++; s++; }

		if (!l && m) {
			lq = q;
			px = s;
			p = this->nodes[q].dat + *s;
			q = *p;
		} else break;
	}

	if (!q) { // Consumió todo s y sobra un caracter o más
		this->nodes[lq].chlds++;
		q = *p = patricia_new_node(this);
		patnode_create(this->nodes+q, lq, px, m);
	} else if (!l && !m) { // Consumió ambos y está en un nodo ya existente
		// Asigna el código y nada más
	} else if (l) {
		// Split de lq---q
		patnode_create(this->nodes+ (this->nodes[lq].dat[*px] = patricia_new_node(this)), lq, px, this->nodes[q].len-l);
		this->nodes[q].len = l;
		this->nodes[q].st = st;
		lq = this->nodes[lq].dat[*px];
		this->nodes[q].prnt = lq;
		this->nodes[lq].chlds++;
		this->nodes[lq].dat[*st] = q;
		if (m) {
			patnode_create(this->nodes+ (q = this->nodes[lq].dat[*s] = patricia_new_node(this)), lq, s, m);
			this->nodes[lq].chlds++;
		} else {
			q = lq;
		}
	}
	if (this->nodes[q].code == LZW_DICTSIZE) this->nodes[q].code = code;
	return q;
}

void patricia_remove_code(patricia* this, uint nd) {
	uint q, lq = 0, l = 0;

	/* Marks the node as removed */
	this->nodes[nd].code = LZW_DICTSIZE;

	if (this->nodes[nd].chlds == 0) { // Leaf-child, remove it
		lq = this->nodes[nd].prnt;
		patnode_delchild(this->nodes + lq, *(this->nodes[nd].st));
		patricia_delete_node(this, nd);
		if ((this->nodes[lq].code == LZW_DICTSIZE) && this->nodes[lq].chlds == 1 && this->nodes[lq].prnt != 0) nd = lq;
	}
	if (this->nodes[nd].chlds == 1) { // Middle-child, remove it and connect parent--child
		forn(q, 256) if (this->nodes[nd].dat[q]) break;
		q = this->nodes[nd].dat[q];
		l = this->nodes[nd].len;
		this->nodes[q].st -= l;
		this->nodes[q].len += l;
		this->nodes[q].prnt = this->nodes[nd].prnt;
		lq = this->nodes[nd].prnt;
		if (lq) {
			this->nodes[lq].dat[*this->nodes[nd].st] = q;
		}
		patricia_delete_node(this, nd);
	}
}

uint patricia_longest_pref(const patricia *this, uchar* s, uint* n) {
	uint l = 0, m = *n, q = this->root, mp = 0, mn = 0;
	uchar *st = 0;
//	DBGcn(s, *n);
	/* Consume prefijos */
	while (q && m) {
		l = this->nodes[q].len;
		st = this->nodes[q].st;
		while (m && l && *st == *s) { l--; m--; st++; s++; }

		if (!l) {
			if (this->nodes[q].code != LZW_DICTSIZE) { mp = this->nodes[q].code; mn = *n - m; }
			if (m) q = this->nodes[q].dat[*s];
		} else break;
	}
	*n = mn;
	return mp;
}

uint patricia_get_len(const patricia* this, uint nd) {
	uint res = 0;
	while (nd != 0) {
		res += this->nodes[nd].len;
		nd = this->nodes[nd].prnt;
	}
	return res;
}

uchar* patricia_get_str(const patricia* this, uint nd) {
	return this->nodes[nd].st - (patricia_get_len(this, nd) - this->nodes[nd].len);
}

uint patricia_copy_str(const patricia* this, uint nd, uchar* s) {
	uint res = patricia_get_len(this, nd);
	if (s) memcpy(s, this->nodes[nd].st - (res- this->nodes[nd].len), res);
	return res;
}

/*** Dict ***/

typedef struct {
	uint mcod;
	uint* codes;
	patricia pat;
	uint *llp, *lln, *pcode;
	uchar *init_codes;
} dict;

void dict_create(dict* this) {
	this->mcod = 0;
	this->codes = (uint*)pz_malloc(LZW_DICTSIZE*sizeof(uint));
	this->pcode = (uint*)pz_malloc((LZW_DICTSIZE+1)*sizeof(uint));
	this->lln = (uint*)pz_malloc((LZW_DICTSIZE+1)*sizeof(uint));
	this->llp = (uint*)pz_malloc((LZW_DICTSIZE+1)*sizeof(uint));
	this->init_codes = NULL;
	patricia_create(&(this->pat), LZW_DICTSIZE);
	memset(this->codes, 0, LZW_DICTSIZE*sizeof(uint));
	/* Inicializa la ll */
	this->lln[LZW_DICTSIZE] = this->llp[LZW_DICTSIZE] = LZW_DICTSIZE;
	this->pcode[LZW_DICTSIZE] = LZW_DICTSIZE;
}

void dict_destroy(dict* this) {
	if (this->codes) { pz_free(this->codes); this->codes = 0; }
	if (this->pcode) { pz_free(this->pcode); this->pcode = 0; }
	if (this->lln) { pz_free(this->lln); this->lln = 0; }
	if (this->llp)  {pz_free(this->llp); this->llp = 0; }
	if (this->init_codes) { pz_free(this->init_codes); this->init_codes = 0; }
	patricia_destroy(&(this->pat));
}

inline void ll_insert(uint* llp, uint* lln, uint code) {
	llp[lln[code] = lln[LZW_DICTSIZE]] = code;
	llp[lln[LZW_DICTSIZE] = code] = LZW_DICTSIZE;
}

inline void ll_remove(uint* llp, uint* lln, uint code) {
	lln[llp[lln[code]] = llp[code]] = lln[code];
	lln[code] = llp[code] = code; /* Permite doble remoción */
}

void dict_use_code(dict* this, uint code) {
	if (code < 256) return;
	ll_remove(this->llp, this->lln, code);
	ll_insert(this->llp, this->lln, code);
}

inline uint dict_lru_code(const dict* this) {
	return this->llp[LZW_DICTSIZE];
}

void dict_insert_code(dict* this, uchar* s, uint l, uint code, uint pcode, uint lru) {
	uint p;
	p = patricia_insert_code(&(this->pat), s, l, code);
//	if (lru) { DBGu((uint64)code); DBGu((uint64)p->code); }
	if (p != this->codes[this->pat.nodes[p].code]) { /* Insertó el nuevo código */
		if (this->mcod == LZW_DICTSIZE) {
			ll_remove(this->llp, this->lln, code);
			patricia_remove_code(&(this->pat), this->codes[code]);
		} else {
			this->mcod++;
		}
		this->codes[code] = p;
		this->pcode[code] = pcode;
	} else { /* El código ya existía, lo 'usa' */
		code = this->pat.nodes[p].code;
		ll_remove(this->llp, this->lln, code);
	}
	if (lru) ll_insert(this->lln, this->llp, code);
	else ll_insert(this->llp, this->lln, code);
}

void dict_remove_code(dict* this, uint code) {
	ll_remove(this->llp, this->lln, code);
	patricia_remove_code(&(this->pat), this->codes[code]);
	this->codes[code] = 0;
	this->mcod--;
}

void dict_init(dict* this) {
	uint i, j, m;
	uchar *br;

	m = LZW_DICTSIZE / 256;
	if (!this->init_codes) {
		this->init_codes = pz_malloc((2*m)*256);
		br = this->init_codes;
		forn(i, m) {
			*(br++) = i;
			forsn(j, i+1, 256) {
				*(br++) = i;
				*(br++) = j;
			}
			*(br++) = i;
		}
	}

	forn(i, 256) dict_insert_code(this, this->init_codes + 2*i, 1, i, LZW_DICTSIZE, 0);
	/* vacía la lista  para no sacar nunca los caracteres simples */
	this->lln[LZW_DICTSIZE] = this->llp[LZW_DICTSIZE] = LZW_DICTSIZE;
	forn(i, 256) this->pcode[i] = LZW_DICTSIZE;

	br = this->init_codes;
	forn(i, m) {
		if (this->mcod < LZW_DICTSIZE) dict_insert_code(this, br++, 2, this->mcod, i, 1);

		forsn(j, i+1, 256) {
			if (this->mcod < LZW_DICTSIZE) dict_insert_code(this, br++, 2, this->mcod, i, 1);
			if (this->mcod < LZW_DICTSIZE) dict_insert_code(this, br++, 2, this->mcod, j, 1);
		}
		br++;
		if (this->mcod == LZW_DICTSIZE) break;
	}
}

void dict_show_codes(const dict* this) {
	uint i, j, k;
	uchar* s;
	forn(i, this->mcod) {
		fprintf(stderr, "[%5u] ==> [%5u] ++ tail == ", i, this->pcode[i]);
		j = patricia_get_len(&(this->pat), this->codes[i]);
		s = patricia_get_str(&(this->pat), this->codes[i]);
		forn(k, j) fprintf(stderr, "%u ", s[k]);
		fprintf(stderr, "\n");
	}
}

uint lzw(uchar* src, ushort* dst, uint n) {
	dict d;
	uint m = 0, lp, llp = 0, cod, lcod = 0;
	if (!n) return 0;

//	DBGun(src, n); DBGcn(src, n);
	dict_create(&d);
	dict_init(&d);

//	dict_show_codes(&d);
//	patricia_show(&(d.pat));

	while (n) {
		lp = n;
		cod = patricia_longest_pref(&(d.pat), src, &lp);
//		DBGu(cod); DBGu(lp);
//		DBGcn(src-llp, llp+lp);
//		DBGcn(src, lp);

		if (dst) *(dst++) = cod; m++;
		dict_use_code(&d, cod);
		if (llp) dict_insert_code(&d, src-llp, llp+lp, dict_lru_code(&d), lcod, 0);
		llp = lp;
		lcod = cod;
		n -= lp;
		src += lp;
//		if (!patricia_invariant(&(d.pat))) exit(1);
//		if (!(m%(32*1024))) fprintf(stderr, "Mem used: %.2f MB  / Mem need:  %.2f MB    \r", (double)patricia_mem_used(&(d.pat))/1024.0/1024.0, (double)patricia_mem_need(&(d.pat))/1024.0/1024.0);
	}
	dict_destroy(&d);

//	if (dst) DBGun(dst-m, m);
	return m;
}

uint ilzw(ushort* src, uchar* dst, uint m) {
	dict d;
	uint n = 0, lp, llp = 0, cod, lcod = 0;
	if (!m) return 0;
	if (!dst) return ilzw_len(src, m);

//	DBGun(src, m);
	dict_create(&d);
	dict_init(&d);

	while (m--) {
		cod = *(src++);
		lp = patricia_copy_str(&(d.pat), d.codes[cod], dst);
//		DBGcn(dst, lp);
//		DBGun(dst, lp);
//		DBGcn(patricia_get_str(&(d.pat), d.codes[cod]), lp);

		dict_use_code(&d, cod);
		if (llp) dict_insert_code(&d, dst-llp, llp+lp, dict_lru_code(&d), lcod, 0);
		llp = lp;
		lcod = cod;
		n += lp;
		dst += lp;
	}
	dict_destroy(&d);

//	DBGun(dst-n, n); DBGcn(dst-n, n);
//	exit(0);
	return n;
}

#define vector_push_back(v) { \
	if (nvec == zvec) { xvec = (uchar**)pz_malloc(2*zvec*sizeof(uchar*)); memset(xvec+zvec, 0, zvec * sizeof(uchar*)); memcpy(xvec, vec, zvec*sizeof(uchar*)); pz_free(vec); vec = xvec; zvec *= 2; } \
	vec[nvec++] = v; }

#define buf_rotate(size) { \
	xbuf = buf; zbuf = ((size)>2*zbuf)?(size):(2*zbuf); \
	buf = (uchar*)pz_malloc(sizeof(uchar)*zbuf); vector_push_back(buf); rbuf = zbuf; \
}

uint ilzw_len(ushort* src, uint m) {
	dict d;
	uint i, n = 0, lp, llp = 0, cod, lcod = 0;
	uint zvec = 64, nvec = 0;
	uchar **vec, **xvec;
	uint zbuf = 512, rbuf;
	uchar *xbuf, *buf;
	if (!m) return 0;

	dict_create(&d);
	dict_init(&d);

	// Vector alloc
	vec = (uchar**)pz_malloc(sizeof(uchar*) * zvec);
	memset(vec, 0, zvec * sizeof(uchar*));

	buf = 0;
	buf_rotate(zbuf);

	while (m--) {
		cod = *(src++);
		lp = patricia_get_len(&(d.pat), d.codes[cod]);
		if (lp > rbuf) { // There's no enought space left for the next code
			buf_rotate(llp+lp);
			if (llp) memcpy(buf, xbuf-llp, llp);
			buf += llp; rbuf -= llp;
		}
		patricia_copy_str(&(d.pat), d.codes[cod], buf);
		buf += lp; rbuf -= lp;

		dict_use_code(&d, cod);
		if (llp) {
			dict_insert_code(&d, buf-llp-lp, llp+lp, dict_lru_code(&d), lcod, 0);
		}
		llp = lp;
		lcod = cod;
		n += lp;
	}
	dict_destroy(&d);

	// Vector free
	forn(i, zvec) if (vec[i]) pz_free(vec[i]);
	pz_free(vec);

	return n;
}
