#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include "tipos.h"
#include <stdio.h>

#define BS_MAX 128

/* Input Bit Stream */

typedef struct str_ibitstream {
	FILE* f;
	uchar bf[BS_MAX];
	uint ubits, rem_bytes;

} ibitstream;

void ibs_create(ibitstream* ibs, FILE* f, uint total_bytes);
void ibs_destroy(ibitstream* ibs);
uint64 ibs_read_uint(ibitstream* ibs, uint bts);
uint ibs_read_bit(ibitstream* ibs);
uint64 ibs_read_bbpb(ibitstream* ibs);

/* Output Bit Stream */

typedef struct str_obitstream {
	FILE* f;
	uchar bf[BS_MAX];
	uint bits, ubits;

} obitstream;

void obs_create(obitstream* obs, FILE* f);
void obs_destroy(obitstream* obs);
void obs_flush(obitstream* obs);
void obs_write_uint(obitstream* obs, uint bts, uint64 data);
void obs_write_bit(obitstream* obs, uint bit);
void obs_write_bbpb(obitstream* obs, uint64 num);

#endif /* __BITSTREAM_H__ */
