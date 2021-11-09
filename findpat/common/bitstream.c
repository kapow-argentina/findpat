#include "bitstream.h"
#include <string.h>
#include <assert.h>

#define bi2by(X) (((X)+7)/8)
#define BS_MAXB (BS_MAX*8)

/***** Output bitstream *****/

void obs_create(obitstream* obs, FILE* f) {
	memset(obs->bf, 0, sizeof(obs->bf));
	obs->f = f;
	obs->bits = 0;
	obs->ubits = 0;
}

void obs_destroy(obitstream* obs) {
	if (obs->ubits) {
		if (!fwrite(obs->bf, sizeof(uchar), bi2by(obs->ubits), obs->f)) perror("ERROR writing bits in obitstream");
		obs->bits += obs->ubits;
		obs->ubits = 0;
	}
}

void obs_flush(obitstream* obs) {
	uint tb = obs->ubits / 8;
	if (!tb) return;
	if (!fwrite(obs->bf, sizeof(uchar), tb, obs->f)) perror("ERROR writing bits in obitstream");
	obs->ubits -= tb*8;
	obs->bits += tb*8;
	if (obs->ubits) obs->bf[0] = obs->bf[tb];
	memset(obs->bf+!!obs->ubits, 0, tb*sizeof(uchar));
}

/* Writes first the LSB bit */
void obs_write_uint(obitstream* obs, uint bts, uint64 data) {
	if (bts > sizeof(data)*8) bts = sizeof(data)*8;
	if (obs->ubits + bts > BS_MAXB) obs_flush(obs);
	uchar *p = obs->bf + obs->ubits / 8;
	uint rm = obs->ubits % 8;
	obs->ubits += bts;
	for(; bts--; rm++) {
		if (rm == 8) { rm = 0; p++; }
		*p |= (data & 1) << rm;
		data >>= 1;
	}
}

void obs_write_bit(obitstream* obs, uint bit) {
	obs_write_uint(obs, 1, bit);
}

/** Writes a number in 2-bit-per-bit encoding, as follows:
 * 0 is 0
 * and each positive binary number of b bits: a_(b-1) ... a_1 a_0,
 * (note that a_(b-1) is the MSB and is always 1 since the number is positive)
 * is encoded as 2*b bits:
 * a_(b-1), 0, a_(b-2) 0, ... , a_1, 0, a_1, 1
 *
 * In this way:
 * 1 is 1 1
 * 2 is 1 0 0 1
 * 3 is 1 0 1 1
 * 4 is 1 0 0 0 0 1
 * 5 is 1 0 0 0 1 1
 */
void obs_write_bbpb(obitstream* obs, uint64 num) {
	if (!num) { obs_write_bit(obs, 0); return; }
	uint64 msb = num;
	while (msb & (msb-1)) msb = msb & (msb-1);
	while (msb) {
		uint bt = ((num & msb)!=0) | (msb*2);
		obs_write_uint(obs, 2, bt);
		msb >>= 1;
	}
}

/***** Input bitstream *****/

#define _min(a, b) ((a)>(b)?(b):(a))

void _ibs_syncbuf(ibitstream* ibs);

void ibs_create(ibitstream* ibs, FILE* f, uint total_bytes) {
	ibs->f = f;
	ibs->ubits = BS_MAXB;
	ibs->rem_bytes = total_bytes;
}

void ibs_destroy(ibitstream* ibs) {
	/* Para dejar el puntero en el lugar correcto del archivo. */
	fseek(ibs->f, ibs->rem_bytes, SEEK_CUR);
}

void _ibs_syncbuf(ibitstream* ibs) {
	uint gap = bi2by(BS_MAXB - ibs->ubits);
	uint toread = _min(ibs->rem_bytes, BS_MAX - gap);
	uint r;
	for (uint i = 0; i < gap; ++i)
		ibs->bf[i] = ibs->bf[BS_MAX - gap + i];
	r = fread(&ibs->bf[gap], sizeof(uchar), toread, ibs->f);
	if (r < toread) perror("WARNING reading less bytes from ibitstream");
	ibs->ubits = ibs->ubits - BS_MAXB + 8 * gap;
	ibs->rem_bytes -= r;
}

uint64 ibs_read_uint(ibitstream* ibs, uint bts) {
	uint64 res = 0;
	if (bts > sizeof(res) * 8) bts = sizeof(res) * 8;
	if (ibs->ubits + bts > BS_MAXB) _ibs_syncbuf(ibs);
	uchar *p = &ibs->bf[ibs->ubits / 8];
	uint shift = 0;
	uint rm = ibs->ubits % 8;
	ibs->ubits += bts;
	for (; bts--; ++rm, ++shift) {
		if (rm == 8) { rm = 0; p++; }
		res |= ((*p >> rm) & 1) << shift;
	}
	return res;
}

uint ibs_read_bit(ibitstream* ibs) {
	return ibs_read_uint(ibs, 1);
}

uint64 ibs_read_bbpb(ibitstream* ibs) {
	if (ibs_read_bit(ibs) == 0) return 0;
	uint64 res = 1;
	uint fin = ibs_read_bit(ibs);
	uint rd;
	while (!fin) {
		rd = ibs_read_uint(ibs, 2);
		res = (res << 1) | (rd & 1);
		fin = rd & 2;
	}
	return res;
}
