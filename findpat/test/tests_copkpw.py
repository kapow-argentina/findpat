#!/usr/bin/python
# vim:et:
from __future__ import with_statement
import os
import random

options_test = {
    'own patterns': ('', False, False),
    'own patterns with nm': ('-nm', True, False),
    'common substrings': ('-c', False, False),

    'own patterns (reversed BWT)': ('', False, True),
    'own patterns with nm (reversed BWT)': ('-nm', True, True),
    'common substrings (reversed BWT)': ('-c', False, True),
}
def fn(i): return '__tmp.%i.fa__' % i
def gen(): return ''.join(random.choice('ACTG') for i in range(random.randint(50, 200)))

PROG1 = 'runcop'
PROG2 = 'copkpw'

tot_err = 0
tot_suc = 0
nt = 0
for optname, (options, sort_it, reverse_bwt) in options_test.items():
    print "--- Testing %s (%s) ---" % (optname, options)
    err = 0
    suc = 0
    for i in range(100):
        print nt,
        nt += 1
        gs = [gen() for i in range(random.randint(2,10))]
        os.system('rm -f __tmp*')
        cmd1 = '%s %s %s >__tmp_out1__ 2>/dev/null' % (PROG1, options, ' '.join(gs))
        os.system(cmd1)
        for i, s in enumerate(gs):
          with file(fn(i), 'w') as g: 
            g.write(s)
          if reverse_bwt:
              if i == 0:
                  os.system('findpatkpw %s /dev/null ./ 40 2>/dev/null' % (fn(0),))
              else:
                  os.system('findpatkpw %s %s ./ 40 2>/dev/null' % (fn(i), fn(0)))
          #else:
          #    os.system('findpatkpw %s %s ./ 40 2>/dev/null' % (fn(0), '/dev/null' if i == 0 else fn(i),))
        cmd2 = '%s %s %s >__tmp_out2__ 2>/dev/null' % (PROG2, options, ' '.join(fn(i) for i in range(len(gs))))
        os.system(cmd2)
        #os.system('cat __tmp_out1__')
        #os.system('cat __tmp_out2__')
        if sort_it:
            os.system('sort __tmp_out1__ > __tmp_out1_sorted__')
            os.system('sort __tmp_out2__ > __tmp_out2_sorted__')
            diff = os.popen('diff __tmp_out1_sorted__ __tmp_out2_sorted__ 2>&1').read()
        else:
            diff = os.popen('diff __tmp_out1__ __tmp_out2__ 2>&1').read()
        if diff != '':
            print ": ERROR - %s and %s differ" % (PROG1, PROG2)
            print "DIFF {"
            print diff
            print "}"
            err += 1
        else:
            print ": OK"
            suc += 1
        os.system('rm -f __tmp*')
    tot_err += err
    tot_suc += suc
    #print "--- Testing of %s finished ---" % (optname,)
    #print "Results:", err, "errors,", suc, "tests OK."
print
print "=== Tests finished ==="
print "Results:", tot_err, "errors,", tot_suc, "tests OK."
