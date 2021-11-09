#!/usr/bin/env python
# coding: utf-8

import argparse
import ktools, kapow

def param_slice(s):
	if re.match("^[0-9]+$", s):
		return (int(s), int(s))
	m = re.match(r"^([0-9]+)-([0-9]+)$", s)
	if m:
		return (int(m.group(1)), int(m.group(2)))
	raise BaseException('Invalid length '+s)

### Command-line definition
parser = argparse.ArgumentParser(add_help=True, 
	description="Concatenates many FASTA files with it's own reverse complement.")
parser.add_argument('output', metavar='<output_file>', type=argparse.FileType('w'),
	help='The output file. - for stdout')
parser.add_argument('files', metavar='<file>', type=str, nargs='+',
	help='One or more FASTA files in .fa, .fa.gz or .fa.bz2 format.')

def get_len(l,u):
	if l == u: return l
	return random.randint(l,u)

if __name__ == "__main__":
	args = parser.parse_args()
	# Load input
	src = ktools.loadManyFasta(args.files)
	# Generate result
	args.output.write(buffer(src))
	args.output.write('N')
	kapow.rev_comp(src)
	args.output.write(buffer(src))
