#ifndef __NSRPH_H__
#define __NSRPH_H__

void fasta_nsrph(char *fn, uint *pn, uchar **ps, uint **pr, uint **pp, uint **ph);
void recalc_nsrph(uint n, uchar *s, uint *r, uint *p, uint *h);
void fasta_load(char *fn, uint *pn, uchar **ps, uint strip);

#endif
