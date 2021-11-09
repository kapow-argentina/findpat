#include <stdio.h>
#include <stdlib.h>

#include "macros.h"
#include "output_callbacks.h"
#include "enc.h"

int output_file(uint l, uint i, uint n, void* vout) {
	FILE* out = (FILE*)vout;
	int wt = 0;
	wt += fwrite(&l, sizeof(uint), 1, out);
	wt += fwrite(&i, sizeof(uint), 1, out);
	wt += fwrite(&n, sizeof(uint), 1, out);
	return wt;
}

void output_file_text(uint l, uint i, uint n, void* vout) {
	FILE* out = (FILE*)vout;
	fprintf(out, "%u %u %u\n", l, i, n);
}

void output_readable(uint l, uint i, uint n, void* vout) {
	uint j;
	output_readable_data* out = (output_readable_data*)vout;
	forn(j,l) fprintf(out->fp,"%c",out->s[out->r[i]+j]);
	fprintf(out->fp," (%u)\n  ", l);
	forn(j,n) fprintf(out->fp, " %d", out->r[i+j]);
	fprintf(out->fp,"\n");
}

void output_readable_po(uint l, uint i, uint n, void* vout) {
	uint j;
	output_readable_data* out = (output_readable_data*)vout;
	forn(j,l) fprintf(out->fp,"%c",out->s[out->r[i]+j]);
	fprintf(out->fp,"\n");
}

void output_readable_trac(uint l, uint i, uint n, void *vout) {
	uint j;
	output_readable_data* out = (output_readable_data*)vout;
	forn(j, l) fprintf(out->fp, "%c", out->s[out->r[i] + j]);
	fprintf(out->fp," #%u (%u)\n ", n, l);
	forn(j, n) {
		uint pos = out->r[i + j];
		if (!out->trac_size) {
			fprintf(out->fp, " <%d", pos);
		} else {
			pos = trac_convert_pos_virtual_to_real(pos, out->trac_buf, out->trac_size);
			if (pos < out->trac_middle)
				fprintf(out->fp, " <%d", pos);
			else
				fprintf(out->fp, " >%d", pos - out->trac_middle);
		}
	}
	fprintf(out->fp,"\n");
}

void output_nothing(uint l, uint i, uint n, void* out) {
}
