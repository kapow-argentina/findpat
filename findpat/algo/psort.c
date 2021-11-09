#include <assert.h>
#include <stdlib.h>
#include "psort.h"
#include "stackarray.h"
#include "macros.h"

typedef void*(psort_thread_t_void)(void*);

/* Thread Manager function */
void psort_init(psort* p, uint nthread, psort_thread_t* psort_f) {
	uint i;
	assert(nthread <= PSORT_MAX_THREAD);
	pthread_mutex_init(&(p->mt), NULL);
	pthread_mutex_init(&(p->wkend), NULL);
	pthread_mutex_lock(&(p->wkend));
	sem_init(&(p->jobp), 0, 0);
	queue_init(p->queue);
	p->texit = FALSE;
	p->nthread = nthread;
	p->wrking = 0;
	forn(i, nthread) {
		pthread_create(p->th+i, NULL, (psort_thread_t_void*)psort_f, p);
	}
}

void psort_destroy(psort* const ps) {
	uint i;
	ps->texit = TRUE;
	pthread_mutex_unlock(&(ps->wkend));
	/* Wake-up all workers (n signals) */
	forn(i, ps->nthread) sem_post(&(ps->jobp));
	/* Wait for all workers to end */
//	forn(i, ps->nthread) pthread_join(ps->th[i], NULL);

	pthread_mutex_destroy(&(ps->mt));
	pthread_mutex_destroy(&(ps->wkend));
	sem_destroy(&(ps->jobp));
}


uint *dbg_st;
void psort_job_new(psort* const ps, const psort_job* jb) {
	dbg_st = jb[0].b;
	queue_push(ps->queue, PSORT_QUEUE_LEN, *jb);
	sem_post(&(ps->jobp));
	pthread_mutex_lock(&(ps->wkend)); // Wait until work ends;
}

/* psort job-queue manager */
__attribute__((always_inline)) bool psort_job_add(psort* ps, const psort_job* jb);
bool psort_job_add(psort* ps, const psort_job* jb) {
	pthread_mutex_lock(&(ps->mt));
	if (queue_full(ps->queue, PSORT_QUEUE_LEN)) {
		pthread_mutex_unlock(&(ps->mt));
		return FALSE;
	}
	queue_push(ps->queue, PSORT_QUEUE_LEN, *jb)
	pthread_mutex_unlock(&(ps->mt));
	sem_post(&(ps->jobp));
	return TRUE;
}

__attribute__((always_inline)) void psort_job_get(psort* ps, psort_job* jb);
void psort_job_get(psort* ps, psort_job* jb) {
	pthread_mutex_lock(&(ps->mt));
	*jb = queue_front(ps->queue);
	queue_pop(ps->queue, PSORT_QUEUE_LEN);
	ps->wrking++;
	pthread_mutex_unlock(&(ps->mt));
}


/** QSORT que parte el arreglo en tres partes: <pivote, == pivote, > pivote */
//#define _qshow(X) { uchar* p = b; fprintf(stderr, "%s[%lX, %lX, %lX)  vp=%c  ", X, (long unsigned int)bp, (long unsigned int)cp, (long unsigned int)ep, vp); for(p=b;p<e;++p) fprintf(stderr, "%c", *p); fprintf(stderr, "\n"); }
//#define _qshow(X) { uint64* p = b; fprintf(stderr, "%s[%lX, %lX, %lX)  vp=%3d  ", X, (long unsigned int)bp, (long unsigned int)cp, (long unsigned int)ep, (int)vp); for(p=b;p<e;++p) fprintf(stderr, "%d ", (int)*p); fprintf(stderr, "\n"); }
#define _qshow(X)

#define SWAP(a,b) {*(a)^=*(b); *(b)^=*(a); *(a)^=*(b);}
#define psort_rec_call(jb, b, e) { if ((b) >= (e)-1) jb = (psort_job){NULL, NULL}; else jb = (psort_job){b,e}; }
#define _def_pqsort3(nombre, tipo, tipoval, VL, OP) \
typedef struct { tipo *st, *ds; } psort_job_##nombre; \
__attribute__((always_inline)) void nombre(psort_job* jb, psort_job* ojb); \
void nombre(psort_job* jb, psort_job* ojb) { \
	tipo *bp, *ep, *cp, *b = (tipo*)(jb->b), *e=(tipo*)(jb->e); \
	tipoval vp, vcp; \
	if (b >= e-1) { jb->b = NULL; return; } \
	cp = b+(rand()%(e-b)); \
	vp = (VL(cp)); /* pivote */\
	bp = cp = b; \
	ep = e; \
	_qshow(">> ") \
	while ((cp < ep) && (vp OP (VL((ep-1))))) --ep; \
	while(cp < ep) { \
		if ((vcp = (VL(cp))) OP vp) { \
			if (bp != cp) { SWAP(bp, cp); } ++bp; ++cp; \
		} else if (vcp == vp) { \
			++cp; \
		} else { \
			ep--; if (cp != ep) SWAP(cp, ep); \
			while ((cp < ep) && (vp OP (VL((ep-1))))) --ep;\
		} \
	} \
	_qshow("-- ") \
	if ((bp-b)<(e-ep)) { psort_rec_call(*jb, b, bp); psort_rec_call(*ojb, ep, e); } \
	else { psort_rec_call(*ojb, b, bp); psort_rec_call(*jb, ep, e); } \
	_qshow("<< ") \
}

/*** QSORT de indices que copia a otra estructura en un vector de elementos de 64bits. ***/
static uint val_pqsortM32(uint64* x) { return *(uint*)x; }
_def_pqsort3(internal_psortM32, uint64, uint, val_pqsortM32, <)

/* Main thread function */
void* psort_thread(psort* ps) {
	psort_job *stjb, *jb;
	uint stjb_p;
	uint stjb_mx = 32;
	stjb = (psort_job*)pz_malloc(stjb_mx * sizeof(psort_job));
	stack_init(stjb);

	while (!ps->texit) {
		sem_wait(&(ps->jobp));
		if (ps->texit) break;

		// Get job from global-job queue
		jb = stjb; stjb_p = 1;
		psort_job_get(ps, jb);

		// Process the global-job
		while (!stack_empty(stjb)) {
//			fprintf(stderr, "{%d} [%3d] job process: [%d, %d)\n", ps->wrking, queue_size(ps->queue, PSORT_QUEUE_LEN), (int)((uint*)jb[0].b-dbg_st), (int)((uint*)jb[0].e-dbg_st));
			if (stack_full(stjb, stjb_mx-1)) {
//				fprintf(stderr, "{%d} [%3d] (%d) Local stack realloc\n", ps->wrking, queue_size(ps->queue, PSORT_QUEUE_LEN), stack_size(stjb));
				stjb_mx *= 2; stjb = realloc(stjb, stjb_mx * sizeof(psort_job));
				jb = &stack_front(stjb);
			}
			internal_psortM32(jb, jb+1);
			if (!jb[0].b && !jb[1].b) { stack_pop(stjb); jb--; } // Caso base
			else if (!jb[0].b) { jb[0] = jb[1]; jb[1] = (psort_job){NULL, NULL}; }
			else if (jb[1].b) {
				/* Dispatch job for queue */
//				fprintf(stderr, "{%d} [%3d] job enqueue: [%d, %d)\n", ps->wrking, queue_size(ps->queue, PSORT_QUEUE_LEN), (int)((uint*)jb[1].b-dbg_st), (int)((uint*)jb[1].e-dbg_st));
				if (((uint*)jb[1].e - (uint*)jb[1].b < PSORT_LOCAL_TRESH) || (!psort_job_add(ps, jb+1))) {
					// Main-queue full, use local stack
//					fprintf(stderr, "{%d} [%3d] (%d) Main queue full!\n", ps->wrking, queue_size(ps->queue, PSORT_QUEUE_LEN), stack_size(stjb));
					stjb_p++; jb++; // Push without copy;
				}
			} /* else: jb[0] is the next job for this thread */
		}
		pthread_mutex_lock(&(ps->mt));
		ps->wrking--;
		if ((ps->wrking == 0) && queue_empty(ps->queue)) pthread_mutex_unlock(&(ps->wkend));
		pthread_mutex_unlock(&(ps->mt));
	}
	pz_free(stjb);
	return NULL;
}
