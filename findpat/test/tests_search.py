#!/usr/bin/python

import os
import random

Alphabet = 'ACTG'

def genstr(n):
  return ''.join([random.choice(Alphabet) for i in range(n)])

def gentest(source_len, ntargets, min_target_len, max_target_len):
  source = genstr(source_len)

  targets = {}
  remaining_targets = ntargets

  obligatory = [
    source[:min_target_len],
    source[:max_target_len],
    source[-min_target_len:],
    source[-max_target_len:],
  ]
  for tg in obligatory:
    if tg not in targets:
      targets[tg] = 1
      remaining_targets -= 1

  for i in range(remaining_targets):
    target = None
    while target == None or target in targets:
      target_len = random.randint(min_target_len, max_target_len)
      if random.randint(0, 99) < 10:
        target = genstr(target_len)
      else:
        start = random.randint(0, source_len - target_len)
        target = source[start:start + target_len]
    targets[target] = 1
  targets = sorted(targets.keys())

  f = file('_genome_.tmp', 'w')
  f.write(source)
  f.close()
  f = file('_reads_.tmp', 'w')
  for target in targets:
    f.write('%s\n' % (target,))
  f.close()

def occurrences_from_pipe(p):
  dic = {}
  while True:
    l1 = p.readline()
    if l1 == '': break
    l2 = p.readline()
    if l2 == '': break
    pat, noccs, len_pat = l1.strip(' \t\r\n').split(' ')
    noccs = int(noccs.strip('#'))
    len_pat = int(len_pat.strip('()'))
    assert len_pat == len(pat)
    if noccs == 0: continue
    occs = map(lambda occ: int(occ.strip('<>')), l2.strip(' \t\r\n').split(' '))
    assert noccs == len(occs)
    all_occs = dic.get(pat, [])
    all_occs.extend(occs)
    dic[pat] = all_occs
  for k in dic.keys():
    dic[k] = sorted(dic[k])
  return dic

def compare_dicts(name1, d1, name2, d2):
  for k in d1.keys():
    if k not in d2:
      print k, 'in', name1, 'but not in', name2
      print d1[k]
      assert False
  for k in d2.keys():
    if k not in d1:
      print k, 'in', name2, 'but not in', name1
      print d2[k]
      assert False
  for k in d1.keys():
    if d1[k] != d2[k]:
      print name1, 'and', name2, 'differ in', k
      print d1[k]
      print d2[k]
      assert False

def occurrences_reference():
  os.system('rm -f _genome_.tmp-null.kpw')
  os.system('findpatkpw --bwt _genome_.tmp /dev/null . 1 2>/dev/null')
  return occurrences_from_pipe(os.popen('patoccs _genome_.tmp-null.kpw _reads_.tmp 2>/dev/null'))

def occurrences_backsearch():
  return occurrences_from_pipe(os.popen('runbacksearch _genome_.tmp _reads_.tmp 2>/dev/null'))

def occurrences_chunksearch(chunk_size, overlap):
  return occurrences_from_pipe(os.popen('chunksearch -c %i -o %i _genome_.tmp _reads_.tmp 2>/dev/null' % (chunk_size, overlap)))

def runtests():
  ntests = 100
  for i in range(ntests):
    print 'test', i
    source_len = random.randint(100, 10000)
    ntargets = random.randint(100, 1000)
    min_target_len = 5
    max_target_len = random.randint(min_target_len, source_len)
    overlap = min(random.randint(max_target_len, 2 * max_target_len), source_len)
    chunk_size = random.randint(overlap, source_len)

    print '  source_len', source_len
    print '  ntargets', ntargets
    print '  min_target_len', min_target_len
    print '  max_target_len', max_target_len
    print '  overlap', overlap
    print '  chunk_size', chunk_size

    gentest(source_len, ntargets, min_target_len, max_target_len)

    ref = occurrences_reference()

    bck = occurrences_backsearch()
    compare_dicts('reference', ref, 'backsearch', bck)

    chk = occurrences_chunksearch(chunk_size, overlap)
    compare_dicts('reference', ref, 'chunksearch', chk)

runtests()

