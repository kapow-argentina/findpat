#ifndef __RMQ_H__
#define __RMQ_H__

#include "tipos.h"

typedef uint* rmq;

rmq* rmq_malloc(uint n);
void rmq_free(rmq* q, uint n);


#define _def_rmq_h(op, one) \
void rmq_init##op(rmq* q, uint* vec, uint n); \
uint rmq_query##op(const rmq* const q, uint a, uint b); \
void rmq_update##op(rmq* q, uint a, uint n); \
uint rmq_nexteq##one(rmq* q, uint i, uint vl, uint n); \
uint rmq_preveq##one(rmq* q, uint i, uint vl, uint n);


_def_rmq_h(_min, _leq)
_def_rmq_h(_max, _geq)

#endif // __RMQ_H__
