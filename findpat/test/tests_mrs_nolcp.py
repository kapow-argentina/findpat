#!/usr/bin/python
import sys
import random
import os

def genstr(al, long):
  return ''.join([random.choice(al) for i in range(long)])

A1 = '01'
A2 = 'ACTG'
A3 = 'abcdefghijklmnopqrstuvwxyz'

n = 0
ok = 0
err = 0
for al in [A1, A2, A3]:
  for ml in [1, 2, 5, 10, 20]:
    for long in [10, 100, 101, 1000, 10000]:
      for i in range(10):
        n += 1
        print 'Test', n,
        s = genstr(al, long)
        os.system('runmrs %s --ml %i | sort > __tmp1__' % (s, ml))
        os.system('runmrs %s --nolcp --ml %i | sort > __tmp2__' % (s, ml))
        diff = os.popen('diff __tmp1__ __tmp2__ 2>&1').read()
        if diff != '':
          err += 1
          print "FAILED"
          print s
          sys.exit(1)
        else:
          ok += 1
          print "OK"
        os.system('rm __tmp1__ __tmp2__')
        sys.stdout.flush()
print n, 'tests run.', ok, 'tests ok.', err, 'tests failed.'
