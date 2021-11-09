#!/usr/bin/python
import sys
import re

_malloc = re.compile(r'([^ ]+) log pz_malloc\(([0-9]+) bytes\) => (0x[0-9a-f]+)')
_free = re.compile(r'([^ ]+) log pz_free\((0x[0-9a-f]+)\)')
nalloc = 0
ndealloc = 0
nmallocs = 0
nfrees = 0
if len(sys.argv) <= 1:
	print "Use: %s <stderr-log>" % (sys.argv[0],)
else:
	f = file(sys.argv[1], "r")
	memoria = {}
	try:
		for line in f:
			line = line.strip(' \t\r\n')
			m = _malloc.search(line)
			if m:
				pos = m.expand('\\1')
				nbytes = int(m.expand('\\2'))
				addr = m.expand('\\3')
				nalloc += nbytes
				nmallocs += 1
				if memoria.has_key(addr):
					print "WARNING: %s - malloc returned an already occuppied address (%s)" % (pos, addr)
				memoria[addr] = (pos, nbytes)
			m = _free.search(line)
			if m:
				pos = m.expand('\\1')
				addr = m.expand('\\2')
				if memoria.has_key(addr):
					(pos, nbytes) = memoria[addr]
					ndealloc += nbytes
					nfrees += 1
					del memoria[addr]
				else:
					print "WARNING: %s - freeing an already free address (%s)" % (pos, addr)
	finally:
		f.close()

	print "-- %i mallocs\t%i bytes allocated" % (nmallocs, nalloc)
	print "-- %i frees\t%i bytes deallocated" % (nfrees, ndealloc)
	print "-- %i lost bytes" % (nalloc - ndealloc,)
	for addr, (pos, nbytes) in memoria.items():
		print "\tunreclaimed malloc at %.30s\t(%.30s)\t%i bytes" % (pos, addr, nbytes)

