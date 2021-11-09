#include <stdio.h>
#include <stdlib.h>

#define tam_buf 1024
unsigned char buf[tam_buf];

int main(int argc, char **argv) {
	int pos, size, rd, i;

	if (argc <= 1) {
		fprintf(stderr,
		"Usage: %s <pos> [<size>]\n"
		"\tOutputs the contents of the standard input from bytes\n"
		"\t<pos> to <pos> + <size>, omitting whitespace in the count.\n"
		, argv[0]);
		return 0;
	}
	rd = 0;
	pos = atoi(argv[1]);
	size = 50;
	if (argc >= 3)
		size = atoi(argv[2]);

	char c = fgetc(stdin);
	if (c == '>') {
		while (fgetc(stdin) != '\n') { /* skip header */ }
	} else {
		ungetc(c, stdin);
	}

	for (;;) {
		rd = fread(buf, 1, tam_buf, stdin);
		if (rd <= 0) goto out;
		for (i = 0; i < rd; ++i) {
			switch (buf[i]) {
			case 'A': case 'C': case 'G': case 'T': case 'N':
			case 'a': case 'c': case 'g': case 't': case 'n':
			if (pos > 0) pos--;
			else {
				size--;
				putchar(buf[i]);
				if (size == 0) goto out;
			}
			break;
			default: break;
			}
		}
	}
out:
	printf("\n");
	return 0;
}

