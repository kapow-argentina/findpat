#ifndef __PSORT_H__
#define __PSORT_H__

#include <pthread.h>
#include <semaphore.h>
#include "queuearray.h"
#include "tipos.h"

#define PSORT_MAX_THREAD 8
#define PSORT_QUEUE_LEN 16
#define PSORT_LOCAL_TRESH 2048

typedef struct str_psort_job {
	void *b, *e;
} psort_job;

typedef struct str_psort {
	bool texit;
	uint nthread;

	sem_t jobp;
	pthread_mutex_t mt, wkend;
	pthread_t th[PSORT_MAX_THREAD];
	psort_job queue_declare(queue, PSORT_QUEUE_LEN);
	uint wrking;

} psort;

typedef void*(psort_thread_t)(psort*);

void psort_init(psort* p, uint nthread, psort_thread_t* psort_f);
void psort_destroy(psort* const p);
void psort_job_new(psort* const ps, const psort_job* jb);


void* psort_thread(psort* ps);


#endif
