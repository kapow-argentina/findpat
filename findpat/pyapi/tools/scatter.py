#!/usr/bin/env python
# coding: utf-8

import argparse, re, os, sys, random
import ktools, kapow

def param_slice(s):
	if re.match("^[0-9]+$", s):
		return (int(s), int(s))
	m = re.match(r"^([0-9]+)-([0-9]+)$", s)
	if m:
		return (int(m.group(1)), int(m.group(2)))
	raise BaseException('Invalid length '+s)

def int_list(s):
	return map(int, filter(None, s.split(',')))

### Command-line definition
parser = argparse.ArgumentParser(add_help=True, 
	description="Randomly selects portions form a set of FASTA files.")
parser.add_argument('output', metavar='<output_file>', type=str,
	help='The output file. - for stdout')
parser.add_argument('files', metavar='<file>', type=str, nargs='+',
	help='One or more FASTA files in .fa, .fa.gz or .fa.bz2 format.')
parser.add_argument('-l', '--len', dest='len', default="20-100", metavar='L[-U]', type=param_slice, 
	help='The length L or length range L-U for the selected portions. (default: 20-100)')
parser.add_argument('-c', '--count', dest='count', default="100", metavar='N', type=int, 
	help='The number N of portions selected. (default: 100)')
parser.add_argument('--sep', dest='sep', default="\n", metavar='C', type=str, 
	help='Separator for diferent portions in the output. (default: \n)')
parser.add_argument('--fastq', '-fq', dest='fastq', action='store_const', const='Q', default='T',
	help='Save the output as FASTQ format instead of plain text.')
parser.add_argument('--fasta', '-fa', dest='fastq', action='store_const', const='A',
	help='Save the output as FASTA format instead of plain text.')
parser.add_argument('--no-N', dest='noN', action='store_const', const=True, default=False,
	help='Don\'t take samples with Ns.')
parser.add_argument('--classify', dest='classify', default=None, metavar='K1,K2,K3,...,Kk', type=int_list, 
	help='If set, classify the occurence count of each pattern in groups [K1,K2) [K2,K3) ... [Kk-1,Kk] until _each_group_ has <count> reads on it.')

def get_len(l,u):
	if l == u: return l
	return random.randint(l,u)

if __name__ == "__main__":
	args = parser.parse_args()
	# Load input
	src = ktools.loadManyFasta(args.files, term='\xff')
	# Output file and configuration
	total_reads = args.count
	if args.classify:
		total_reads *=  len(args.classify)-1
		cnts = [0 for i in range(len(args.classify)-1)]
		outfiles = [open(args.output % k, 'w') for k in args.classify[:-1]]
		cache_fn = '_'.join(map(os.path.basename, args.files)) + '.bs'
		if os.path.exists(cache_fn):
			sys.stderr.write("Loading index...\n")
			idx = kapow.BackSearch(open(cache_fn, 'rb'))
		else:
			sys.stderr.write("Building index... ")
			sa = kapow.SAIndex(src, lookup=0, flags=kapow.SA_NO_LRLCP + kapow.SA_NO_SRC)
			idx = kapow.BackSearch(src, sa.get_r())
			idx.store(open(cache_fn, 'wb'))
			sys.stderr.write("[Done]\n")
	else:
		outfile = open(args.ouptut, 'w')
		
	# Select queries and write output
	for c in xrange(total_reads):
		ok = False
		while not ok:
			l = get_len(*args.len)
			# Scatter a piece
			i = random.randint(0,len(src)-l-2)
			s = src[i:i+l]
			if args.classify[0] <= 0 and cnts[0] < args.count:
				# Generate a random piece
				a = map(chr,s)
				random.shuffle(a)
				s = kapow.Array()
				s.bindFrom(''.join(a))
			ok = not args.noN or kapow.frequency(s)[ord('N')] == 0
			if ok and args.classify:
				st, ed, _ = idx.search(s)
				knt = ed-st
				if knt < args.classify[0] or knt >= args.classify[-1]:
					ok = False
				else:
					k = 0
					while knt >= args.classify[k+1]: k+=1
					ok = cnts[k] < args.count

		if c % 100000 == 0: print "c =", c
		if args.classify:
			outfile = outfiles[k]
			cnts[k] += 1
		if args.fastq == 'Q':
			outfile.write('@pos%d/l%d\n' % (i,l))
			outfile.write(buffer(s))
			outfile.write('\n+pos%d/l%d\n%s\n' % (i,l,'#'*l))
		elif args.fastq == 'A':
			outfile.write('> pos%d/l%d\n' % (i,l))
			outfile.write(buffer(s))
			outfile.write('\n')
		else:
			outfile.write(buffer(s))
			if len(args.sep):
				outfile.write(args.sep)
