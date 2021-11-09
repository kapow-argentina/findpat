#ifndef __FASTQ_H__
#define __FASTQ_H__

#define fastq_it(it) it##_lpt
#define fastq_len(it) (it##_pt-it##_lpt)
#define for_fastq(it, src, n) \
	for(uchar* it##_pt=(src), *it##_lpt=NULL, it##_rd=0; \
		it##_pt < ((uchar*)(src))+(n); \
		(*it##_pt=='\n')?((it##_rd=(it##_rd+1)%4),(it##_lpt=++it##_pt)):it##_pt++) \
		if (it##_rd==1 && *it##_pt == '\n')

#define line_it(it) it##_lpt
#define line_len(it) (it##_pt-it##_lpt)
#define for_line(it, src, n, sep) \
	for(uchar* it##_pt=(src), *it##_lpt=(src); \
		it##_pt < ((uchar*)(src))+(n); \
		(*it##_pt==(sep))?(it##_lpt=++it##_pt):++it##_pt) \
		if (*it##_pt == (sep))


#endif
