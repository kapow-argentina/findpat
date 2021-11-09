#ifndef __INTERV_H__
#define __INTERV_H__

#include "tipos.h"

typedef struct _interv interv;
struct _interv {
	uint n;
	uint cov[];
};

typedef void (*interv_callback)(uint start, uint end);

interv *interv_new(uint n);
void interv_destroy(interv *in);
void interv_add(interv *in, uint a, uint b);
void interv_union(interv *in, interv_callback cb);

#endif /* __INTERV_H__ */
