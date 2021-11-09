#ifndef __RMQBIT_H__
#define __RMQBIT_H__

#include "tipos.h"

typedef uint* rmqbit;

rmqbit* rmqbit_malloc(uint n);
void rmqbit_free(rmqbit* q, uint n);
uint rmqbit_val(const rmqbit* const q, uint a, uint l);

#define _def_rmqbit_h(op) \
void rmqbit_init##op(rmqbit* q, uint* vec, uint n); \
uint rmqbit_query##op(const rmqbit* const q, uint a, uint b); \
void rmqbit_update##op(rmqbit* q, uint a, uint n);


#ifdef _DEBUG
void rmqbit_show(rmqbit* q, uint n);
void rmqbit_bitshow(rmqbit* q, uint n);
#endif

_def_rmqbit_h(_min)
_def_rmqbit_h(_max)

#endif // __RMQBIT_H__
