#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tipos.h"
#include "enc.h"
#include "common.h"
#include "macros.h"

#define SEPARATOR '~'

int main(int argc, char** argv) {
	uchar *content, *real_content;
	uint length, real_length, final_length;
	
	if (argc < 3) {
		fprintf(stderr, "Usage: %s file1.fa[.gz] file2.fa[.gz] ... fileN.fa[.gz] file.out\n", argv[0]);
		return 1;
	}

	FILE* out = fopen(argv[argc - 1], "wb");
	if (!out) {
		fprintf(stderr, "Cannot open %s for writing\n", argv[2]);
		return 1;
	}
	
	int i;
	forsn (i, 1, argc - 1) {
		bool gzipped = FALSE;
		char *fn = argv[i];
		int l = strlen(fn);
		if (l > 3 && !strcmp(&fn[l - 3], ".gz")) {
			gzipped = TRUE;
		}

		content = gzipped ? loadStrGzFile(fn, &length) : loadStrFile(fn, &length);
		if (!content) {
			fprintf(stderr, "Cannot open %s for reading\n", fn);
			fclose(out);
			return 1;
		}

		real_content = fa_strip_header(content, length);
		real_length = length - (real_content - content);
		
		final_length = fa_strip_n_and_blanks(real_content, real_content, real_length);
		
		if (!fwrite(real_content, final_length, sizeof(uchar), out)) {
			fprintf(stderr, "Writing on %s failed\n", argv[2]);
			fclose(out);
			free(content);
			return 1;
		}

		if (i != argc - 2) {
			fprintf(out, "%c", SEPARATOR);
		}
		
		free(content);
	}
	fclose(out);
	return 0;
}

