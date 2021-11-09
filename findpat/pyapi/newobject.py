#!/usr/bin/python
import sys
import os

if len(sys.argv) != 2:
  sys.stderr.write('Usage: %s <ObjectName>\n' % (sys.argv[0],))
  sys.exit(1)

obj = sys.argv[1]

for ext in ['c', 'h']:
  lobj = obj.lower()
  uobj = obj.upper()
  out = 'k%s.%s' % (lobj, ext)
  if os.path.isfile(out):
    sys.stderr.write('Error: %s exists, will not overwrite\n' % (out,))
    sys.exit(1)

  f = file('_template.%s' % (ext,), 'r')
  contents = f.read()
  f.close()

  contents = contents.replace('<obj>', lobj)
  contents = contents.replace('<Obj>', obj)
  contents = contents.replace('<OBJ>', uobj)

  g = file(out, 'w')
  g.write(contents)
  g.close()

