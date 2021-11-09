#!/usr/bin/python
import sys
import popen2

def usage():
  sys.stderr.write("Usage: %s file.kpw <options>\n" % (sys.argv[0],))
  sys.stderr.write("Description:\n")
  sys.stderr.write("    Creates the smal file associated to a kpw file.\n\n")
  sys.stderr.write("Options:\n")
  sys.stderr.write("    -L | -R | -2     create smal for patterns in left/right/both files\n")

def filtrar_linea(l):
  izq = False
  der = False
  l = l.strip(' \t\r\n').split(' ')
  for x in l:
    if x[0] == '<': izq = True
    elif x[0] == '>': der = True
    else:
      raise Exception('!!! Archivo mal formado')
  if izq and der:
    return '2'
  elif izq:
    return 'L'
  elif der:
    return 'R'
  else:
    return ''

if len(sys.argv) < 2 or len(sys.argv) > 3:
  usage()
  sys.exit(1)

opt = ''
suf = ''
if len(sys.argv) >= 3:
  if sys.argv[2] == '-L': suf = 'L'
  elif sys.argv[2] == '-R': suf = 'R'
  elif sys.argv[2] == '-2': suf = '2'
  else:
    sys.stderr.write('!!! Unknown option: %s\n' % (sys.argv[2],))
    usage()
    sys.exit(1)
  opt = '-' + suf

def es_suf(x, y):
  return len(x) >= len(y) and x[-len(y):] == y

def es_pref(x, y):
  return len(x) >= len(y) and x[:len(y)] == y

def strip_suf(x, y):
  if not es_suf(x, y):
    return x
  else:
    return x[:-len(y)]

if es_suf(sys.argv[1], '.txt'):
  o = file(sys.argv[1], 'r')
  bare = strip_suf(sys.argv[1], '.txt')
  filtrar = True 
else:
  (o, i) = popen2.popen2('kapow dumptext %s %s:PATT' % (opt, sys.argv[1]))
  bare = strip_suf(sys.argv[1], '.kpw')
  filtrar = False 

dic = {}
while True:
  l = o.readline()
  if l == '': break
  l = l.split(' ')
  if len(l) == 3:
    long = int(l[2].strip('()\r\n'))
    occs = int(l[1].strip('#'))
    l = o.readline()
    if l == '': break
    if filtrar and suf != '':
      fl = filtrar_linea(l)
      if   suf == '2' and fl != '2': continue
      elif suf == 'L' and fl != 'L': continue
      elif suf == 'R' and fl != 'R': continue
  elif len(l) == 1:
    long = len(l[0].strip(' \t\r\n'))
    occs = 1
  else:
    sys.stderr.write("!!! Error: line could not be matched.")
    sys.exit(1)
  clave = (long, occs)
  dic[clave] = dic.get(clave, 0) + 1

sname = 'smal%s-%s.txt' % (suf, bare)
sys.stderr.write('Dumping smal to <%s>\n' % (sname,))
f = file(sname, 'w')
f.write('len\treps\t#pats\n')
ks = dic.keys()
ks.sort()
for k in ks:
  len, reps = k
  pats = dic[k]
  f.write('%i\t%i\t%i\n' % (len, reps, pats))
f.close()

