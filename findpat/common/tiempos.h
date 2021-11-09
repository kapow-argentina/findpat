#ifndef __TIEMPOS_H__
#define __TIEMPOS_H__

#include <sys/time.h>
//#include <sys/times.h>

typedef struct timeval tiempo_usec;
//typedef struct tms tiempo_csec;

typedef tiempo_usec tiempo;

//void getTickTime(tiempo_csec *buf);
//double getTimeDiff(tiempo_csec t_ini, tiempo_csec t_fin);

void getTickTime(tiempo_usec *buf);
double getTimeDiff(tiempo_usec t_ini, tiempo_usec t_fin);

/* Adds the time of running BLOCK to TimeVar */
#define MEASURE_TIME(TimeVar, BLOCK) { \
	tiempo_usec __##TimeVar##_start, __##TimeVar##_end; \
	getTickTime(&__##TimeVar##_start); \
	BLOCK; \
	getTickTime(&__##TimeVar##_end); \
	(TimeVar) += getTimeDiff(__##TimeVar##_start, __##TimeVar##_end);\
}

#endif
