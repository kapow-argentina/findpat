#!/usr/bin/env python
# coding: utf-8

import kapow, ktools
import argparse, re, random

### Command-line definition
parser = argparse.ArgumentParser(add_help=True, 
	description="Run findpat a source")
parser.add_argument('output', metavar='<output_kpw>', type=argparse.FileType('w'),
	help='The output filename as KPW.')
parser.add_argument('files', metavar='<file>', type=str, nargs='+',
	help='One or more FASTA files in .fa, .fa.gz or .fa.bz2 format.')
parser.add_argument('-ml', '--min-length', dest='ml', default="1", metavar='L', type=int, 
	help='The minimum length L to report a repeat.')
parser.add_argument('--sep', dest='sep', default="N", metavar='C', type=str, 
	help='Separator used to glue the files, and used as a terminator. (default: N)')


if __name__ == "__main__":
	args = parser.parse_args()
	# Load input
	src = ktools.loadManyFasta(args.files, sep=args.sep)
	print src
