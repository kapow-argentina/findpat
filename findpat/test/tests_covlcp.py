#!/usr/bin/python
import sys
import random
import os

OPT_SEP = '--sep'

det_strings = [
  "x","xx","xy","yx","mississipi","abcdabcdabcdabcd",
  "abcdabcdabcdabcdefgh","aaaaaaaaaaaaaaa","aaaaaaaaaaaab","baaaaaaaa",
  "abababababab","aaaaabaaaaabaaaaabaaaaab",
  "abcdefghijklmnopqrstuvwxyz",
  "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  "aba","aab","baa","abc","bca","acb","bac","cab","cba",
  "abaaba","aabaab","baabaa","abcabc","bcabca","acbacb","bacbac",
  "cabcab","cbacba",
  "abaabaaba","aabaabaabaab","baabaabaabaa","abcabcabcabc",
  "bcabcabcabca","acbacbacbacb","bacbacbacbacbac","cabcabcabcab",
  "cbacbacbacba","abaabaaba","aabaabaabaaba","baabaabaabaaba",
  "abcabcabcabca","bcabcabcabcab","acbacbacbacbac","bacbacbacbacbacb",
  "cabcabcabcabc","cbacbacbacbac",
  "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyza",
  "zxywvutsrqponmlkjihgfedcbazxywvutsrqponmlkjihgfedcba",
  "zxywvutsrqponmlkjihgfedcbaabcdefghijklmnopqrstuvwxyz",
  "unkcvxyvwfxerpwhgwwonatogeuyaouudgwbdwwb",
  "cdccdcbddbcbcbdbaecccdcdddcbcdeac",
  "rrrllleee", "rrrrlllleeee", "rrrrrleeeee",
  "a" * 50, "a" * 49 + "bbbbccccde", "a" * 64, "abc" * 31
]

def m_reference(s):
  m = [0 for i in range(len(s))]
  r = os.popen('runmrs %s' % (s,))
  while True:
    l1 = r.readline()
    if l1 == '': break
    l2 = r.readline()
    if l2 == '': break
    l1 = l1.strip(' \t\r\n')
    l2 = l2.strip(' \t\r\n')
    st, ln = l1.split(' ')
    ln = int(ln.strip('()'))
    assert len(st) == ln
    for i in map(int, l2.split(' ')):
      for j in range(ln):
        m[i + j] = max(m[i + j], ln)
  return m

def m_implementation(s):
  os.system('rm -f __tmp__ __tmp__-null.kpw')
  f = file('__tmp__', 'w')
  f.write(s)
  f.close()
  os.system('findpatkpw %s +f --bwt __tmp__ /dev/null . 1 2>/dev/null' % (OPT_SEP,))
  r = os.popen('covlcp -b __tmp__-null.kpw 2>/dev/null')
  lst = r.readline()
  lst = lst.strip(' \t\r\n').split(' ')
  lst = map(int, lst)
  assert lst[-1] == 0
  assert lst[-2] == 0
  lst = lst[:-2]
  return lst

def m_reference2(s, t):
  m1 = [0 for i in range(len(s))]
  m2 = [0 for i in range(len(t))]
  r = os.popen('runmrs %s}%s' % (s, t))
  while True:
    l1 = r.readline().strip(' \t\r\n')
    if l1 == '': break
    l2 = r.readline().strip(' \t\r\n')
    if l2 == '': break
    st, ln = l1.split(' ')
    ln = int(ln.strip('()'))
    assert len(st) == ln
    occs = map(int, l2.split(' '))
    occs1 = []
    occs2 = []
    for o in occs:
      if o > len(s):
        occs2.append(o - len(s) - 1)
      else:
        occs1.append(o)
    if occs1 == [] or occs2 == []:
      # full2
      continue
    for i in occs1:
      for j in range(ln):
        m1[i + j] = max(m1[i + j], ln)
    for i in occs2:
      for j in range(ln):
        m2[i + j] = max(m2[i + j], ln)
  return (m1, m2)

def m_implementation2(s, t):
  os.system('rm -f __tmp1__ __tmp2__ __tmp1__-__tmp2__.kpw')
  f = file('__tmp1__', 'w')
  f.write(s)
  f.close()
  f = file('__tmp2__', 'w')
  f.write(t)
  f.close()
  os.system('findpatkpw %s +f --bwt __tmp1__ __tmp2__ . 1 2>/dev/null' % (OPT_SEP,))
  r = os.popen('covlcp -2 -b __tmp1__-__tmp2__.kpw 2>/dev/null')
  lst = r.readline()
  lst = lst.strip(' \t\r\n').split(' ')
  #print s, t
  #print lst
  lst = map(int, lst)
  assert lst[len(s)] == 0
  assert lst[-1] == 0
  return (lst[:len(s)], lst[len(s) + 1:-1])

nt = 0
def test1(cadena):
  global nt
  nt += 1
  ref = m_reference(cadena)
  imp = m_implementation(cadena)
  sys.stdout.write('.')
  sys.stdout.flush()
  if ref != imp:
    print 'FAILED'
    print 'string', cadena
    print 'ref', ref
    print 'imp', imp
    assert False

def test2(cadena1, cadena2):
  global nt
  nt += 1
  ref = m_reference2(cadena1, cadena2)
  imp = m_implementation2(cadena1, cadena2)
  sys.stdout.write('.')
  sys.stdout.flush()
  if ref != imp:
    print 'FAILED'
    print 'string1', cadena1
    print 'string2', cadena2
    print 'ref', ref
    print 'imp', imp
    assert False

def main_test1():
  print 'test1'
  for x in det_strings:
    test1(x)
  for al in ['01', 'ACTG', 'abcdefghijklmnopqrstuvwxyz']:
    for l in range(1, 10) + [50, 100, 500, 1000]:
      for j in range(10):
	cadena = ''.join([random.choice(al) for i in range(l)])
	test1(cadena)

def main_test2():
  print
  print 'test2'
  #for x in det_strings:
  #  for y in det_strings:
  #    test2(x, y)
  #for al in ['01', 'ACTG~', 'abcdefghijklmnopqrstuvwxyz']:
  for al in ['ACTG~']:
    for l in range(1, 10) + [50, 100, 500, 1000]:
      for j in range(10):
	cad1 = ''.join([random.choice(al) for i in range(l)])
	cad2 = ''.join([random.choice(al) for i in range(l)])
        test2(cad1, cad2)

#main_test1()
main_test2()
print
print 'Done.', nt, 'tests OK.'

