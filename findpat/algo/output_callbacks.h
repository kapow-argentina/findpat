#ifndef __OUTPUT_CALLBACKS_H__
#define __OUTPUT_CALLBACKS_H__

#include "tipos.h"
#include <stdio.h>

/**
 * The first parameter is the length of all repetitions, the second parameter
 * is the index of the position in the suffix array of the first suffix of the
 * input string that has one of the substrings as a prefix. 
 * The third parameter is the number of such repetitions. The fourth is just
 * an echo of the void* passed to the function.
 */
typedef void(output_callback)(uint, uint, uint, void*);

/**
 * A useful output_callback to throw the output on a file pointed by the
 * extra data.
 */
int output_file(uint l, uint i, uint n, void* out);

/**
 * Similar to the above, but writes the file in text mode (easier to read by
 * a human)
 */
void output_file_text(uint l, uint i, uint n, void* out);

typedef struct str_output_readable_data {
	uint* r;
	uchar* s;
	FILE* fp;

	/* For tracking positions */
	uint *trac_buf;
	uint trac_size;
	uint trac_middle;
} output_readable_data;

void output_readable(uint l, uint i, uint n, void* out);

/**
 * Similar to the above, but prints the patterns without further information
 */
void output_readable_po(uint l, uint i, uint n, void* out);

/* Also track positions */
void output_readable_trac(uint l, uint i, uint n, void* out);


/**
 * Do nothing
 */
void output_nothing(uint l, uint i, uint n, void* out);

#endif // __OUTPUT_CALLBACKS_H__
