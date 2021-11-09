
#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"

/* States */
#define S_IN_FASTA  1
#define S_IN_HEADER 2

char *header_filename(char *fn) {
	char *header_fn = malloc(strlen(fn) + 8);
	sprintf(header_fn, "%s.contig", fn);
	return header_fn;
}

#define TAM_BUF			8192
#define FLUSH_BUF_THRESH	8190
char _buffer[TAM_BUF];
#define push_buf(C) 	{ _buffer[bufpos++] = (C); ++write_pos; }

#define CLEANUP { \
	if (f) fclose(f); \
	if (header_f) fclose(header_f); \
	if (header_fn) free(header_fn); \
}
int extract_headers(char *fn, int delete) {
	FILE *f = NULL, *header_f = NULL;
	char *header_fn = NULL;
	
	/* Open files */
	f = fopen(fn, "r+");
	if (!f) {
		fprintf(stderr, "!!! Aborting.\n!!! Cannot open <%s> for reading/writing.\n", fn);
		CLEANUP
		return 0;
	}

	if (!delete) {
		header_fn = header_filename(fn);
		if (fileexists(header_fn)) {
			fprintf(stderr, "!!! Aborting.\n!!! Header file <%s> already exists. Won't overwrite. Remove it to force.\n", header_fn);
			CLEANUP
			return 0;
		}
		header_f = fopen(header_fn, "w");
		if (!header_f) {
			fprintf(stderr, "!!! Aborting.\n!!! Cannot open <%s> for writing.\n", header_fn);
			CLEANUP
			return 0;
		}
	}
	
	/* Remove headers */
	uint state = S_IN_FASTA;
	uint write_pos = 0;
	uint bufpos = 0;
	uint write_off = 0;
	for (;;) {
		int c = fgetc(f);
		if (bufpos >= FLUSH_BUF_THRESH || (c == EOF && bufpos > 0)) {
			/* Flush buffer to file */
			long read_off = ftell(f);
			fseek(f, write_off, SEEK_SET);
			if (fwrite(_buffer, sizeof(char), bufpos, f) != bufpos) {
				fprintf(stderr, "!!! Warning: couldn't write bytes to stream.\n");
			}
			write_off += bufpos;
			fseek(f, read_off, SEEK_SET);
			bufpos = 0;
		}
		if (c == EOF) break;
		switch (state) {
		case S_IN_FASTA:
			if (c == '>') {
				state = S_IN_HEADER;
				if (!delete) fprintf(header_f, "%u >", write_pos);
				fprintf(stderr, "*** Found header at %u.\n", write_pos);
				if (write_pos != 0) {
					/* Reserve one 'N' for initial newline if it's not the first line */
					push_buf('N');
				}
				push_buf('N');
			} else if (c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N') {
				push_buf(c);
			}
			break;
		case S_IN_HEADER:
			if (c == '\n') {
				if (!delete) putc('\n', header_f);
				/*putc('\n', stderr);*/
				state = S_IN_FASTA;
				push_buf('N');
			} else {
				if (!delete) putc(c, header_f);
				/*putc(c, stderr);*/
				push_buf('N');
			}
			break;
		default:
			assert(0);
		}
	}
	rewind(f);
	if (ftruncate(fileno(f), write_off)) {
		fprintf(stderr, "!!! Warning: cannot truncate file <%s>.\n", fn);
	}

	if (!delete) fprintf(stderr, "*** Wrote <%s>\n", header_fn);
	CLEANUP
	return 1;
}
#undef CLEANUP

#define CLEANUP { \
	if (f) fclose(f); \
	if (header_f) fclose(header_f); \
	if (header_fn) free(header_fn); \
}
#define Errmsg(...) { \
	fprintf(stderr, __VA_ARGS__); \
	if (f) fprintf(stderr, "\tat <%s>:%lu\n", fn, ftell(f)); \
	if (header_f) fprintf(stderr, "\tat <%s>:%lu\n", header_fn, ftell(header_f)); \
}
int recover_headers(char *fn, int overwrite) {
	FILE *f = NULL, *header_f = NULL;
	char *header_fn = NULL;

	/* Open files */
	f = fopen(fn, (overwrite ? "r+" : "r"));
	if (!f) {
		Errmsg("!!! Aborting.\n!!! Cannot open <%s> for reading/writing.\n", fn);
		CLEANUP
		return 0;
	}
	
	header_fn = header_filename(fn);
	if (!fileexists(header_fn)) {
		Errmsg("!!! Aborting.\n!!! Header file <%s> doesn't exist.\n", header_fn);
		CLEANUP
		return 0;
	}

	header_f = fopen(header_fn, "r");
	if (!header_f) {
		Errmsg("!!! Aborting.\n!!! Cannot open <%s> for reading.\n", header_fn);
		CLEANUP
		return 0;
	}

	for (;;) {
		int c = fgetc(header_f);
		if (c == EOF) break;
		ungetc(c, header_f);
		uint hd_off;
		if (fscanf(header_f, "%u", &hd_off) != 1) {
			Errmsg("!!! Aborting.\n!!! Cannot read from <%s>.\n", header_fn);
			CLEANUP
			return 0;
		}
		if (fgetc(header_f) != ' ') {
			Errmsg("!!! Warning: header file is not well formed. Expected ' '.\n");
		}

		/* Write initial newline (if it is not the first line) */
		if (hd_off != 0) {
			fseek(f, hd_off, SEEK_SET);
			if (fgetc(f) != 'N') {
				Errmsg("!!! Warning: overwriting a character != 'N'.\n");
				if (!overwrite) { CLEANUP return 0; }
			}
			if (overwrite)  {
				fseek(f, -1, SEEK_CUR);
				fputc('\n', f);
				++hd_off;
			}
		}

		/* Write rest of the header */
		if (overwrite) {
			fprintf(stderr, "*** Patching header at %u\n", hd_off);
		} else {
			fprintf(stderr, "*** Checking header at %u\n", hd_off);
		}
		uint bufpos = 0;
		for (;;) {
			int c = fgetc(header_f);
			if (bufpos == TAM_BUF || (c == '\n' && bufpos > 0)) {
				/* Check that we're overwriting only Ns */
				fseek(f, hd_off, SEEK_SET);
				uint i;
				for (i = 0; i < bufpos; ++i) {
					if (fgetc(f) != 'N') {
						Errmsg("!!! Warning: overwriting a character != 'N'.\n");
						if (!overwrite) { CLEANUP return 0; }
					}
				}
				/* Overwrite them */
				if (overwrite) {
					fseek(f, hd_off, SEEK_SET);
					if (fwrite(_buffer, sizeof(char), bufpos, f) != bufpos)
						Errmsg("!!! Warning: couldn't write bytes to stream.\n");
				}
				if (c == '\n') {
					if (fgetc(f) != 'N') {
						Errmsg("!!! Warning: overwriting a character != 'N'.\n");
						if (!overwrite) { CLEANUP return 0; }
					}
					if (overwrite) {
						fseek(f, -1, SEEK_CUR);
						fputc('\n', f);
					}
				}
				hd_off += bufpos;
				bufpos = 0;
			}
			if (c == '\n') break;
			_buffer[bufpos++] = c;
		}
	}

	CLEANUP
	return 1;
}
#undef CLEANUP

void usage(char *p) {
	fprintf(stderr, "Usage: %s <options> <fasta-file>\n", p);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "       -x       extract headers into a .contig file and replace them by Ns\n");
	fprintf(stderr, "       -d       delete headers and replace them by Ns (don't create .contig file)\n");
	fprintf(stderr, "       -r       recover headers from .contig file\n");
	fprintf(stderr, "       -t       test if headers from .contig file are well formed\n");
}

int main(int argc, char **argv) {
	if (argc != 3) {
		usage(argv[0]);
		exit(1);
	}
	
	if (0) {
	} else if (!strcmp(argv[1], "-x")) {
		extract_headers(argv[2], 0);
	} else if (!strcmp(argv[1], "-d")) {
		extract_headers(argv[2], 1);

	} else if (!strcmp(argv[1], "-t")) {
		if (recover_headers(argv[2], 0)) {
			fprintf(stderr, "*** Headers are well formed.\n");
		} else {
			fprintf(stderr, "!!! Headers are NOT well formed.\n");
		}
	} else if (!strcmp(argv[1], "-r")) {
		recover_headers(argv[2], 1);
	} else {
		fprintf(stderr, "Unknown option.\n");
		usage(argv[0]);
		exit(1);
	}
	return 0;
}

