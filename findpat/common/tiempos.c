#include "tiempos.h"

#include <unistd.h>

// usando times()
/*
#define CLK_TCK (sysconf(_SC_CLK_TCK))

void getTickTime(tiempo_csec *buf) {
	clock_t t;
	t = times(buf);
	return;
}

double getTimeDiff(tiempo_csec t_ini, tiempo_csec t_fin) {
	clock_t t = t_fin.tms_utime;
	t -= t_ini.tms_utime;
	return t * 1000 / (double)CLK_TCK;
}
*/

// usando gettimeofday()

void getTickTime(tiempo_usec *buf) {
	gettimeofday(buf, 0);
	return;
}

double getTimeDiff(tiempo_usec t_ini, tiempo_usec t_fin) {
	unsigned long int t = (unsigned long int)t_fin.tv_sec * 1000000 + t_fin.tv_usec;
	t -= (unsigned long int)t_ini.tv_sec * 1000000 + t_ini.tv_usec;
	return t / (double)1000;
}

