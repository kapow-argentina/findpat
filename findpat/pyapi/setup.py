from distutils.core import setup, Extension
from glob import glob
import os, sys

kpw_debug=False

SVN_REVISION=0

# Check for kapow-source
KAPOWPATH='ksrc'
if not os.path.exists(KAPOWPATH):
	os.symlink('../', KAPOWPATH)

CFLAGS=['-Wno-unused-function', '-I%s/algo'%KAPOWPATH, '-I%s/lib'%KAPOWPATH, '-I%s/common'%KAPOWPATH, '-I%s/'%KAPOWPATH, '-std=c99']
if kpw_debug:
	CFLAGS.append('-D_DEBUG')
	CFLAGS.append('-O0')

SRCS = {
	'common': ['tiempos.c', 'common.c', 'bitstream.c'],
	'algo': ['bwt.c', 'lcp.c', 'mtf.c', 'klcp.c', 'enc.c', 'arit_enc.c', \
	         'mrs.c', 'mrs_nolcp.c', 'mmrs.c', 'output_callbacks.c', 'bittree.c', \
	         'rmq.c', 'ranksel.c', 'sa_search.c', 'backsearch.c', 'comprsa.c', \
	         'saindex.c', 'sais.c'],
	'lib': ['kpw.c', 'kpwapi.c']
}
KAPOW_SRC = ['karray.c', 'krmq.c', 'kpwfile.c', 'kcallback.c', 'kranksel.c', 'kbacksearch.c', 'kcomprsa.c', 'ksaindex.c', 'kapow.c']
KAPOW_HEADERS = ['config.h', 'common/macros.h', 'common/tipos.h', 'lib/libp2zip.h', 'algo/sorters.h', 'algo/bitarray.h', 'algo/fastq.h']
KAPOW_DEPS = glob('py_*.c') + glob('k*.h') + ['setup.py'] + map(lambda x: '%s/%s' % (KAPOWPATH, x), KAPOW_HEADERS)

KAPOW_PY = ['ktools']

# Build de SRCs array
for d in SRCS:
	for f in SRCS[d]:
		KAPOW_SRC.append('%s/%s/%s' % (KAPOWPATH,d,f))
		hfile = '%s/%s/%s.h' % (KAPOWPATH,d,f[:-2])
		if f[-2:] == '.c' and os.path.exists(hfile):
			KAPOW_DEPS.append(hfile)

if __name__ == "__main__":
	if len(sys.argv) > 1 and sys.argv[1] == 'list':
		# List package files
		print ' '.join(KAPOW_SRC + KAPOW_DEPS), ' '.join(map(lambda x: x+'.py', KAPOW_PY))
		sys.exit(0)

kapow = Extension('kapow',
	sources = KAPOW_SRC,
	define_macros=[('GZIP', None), ('_PYAPI', None)],
	extra_compile_args=CFLAGS,
	extra_link_args=['-Wl,--version-script=kapow.version'],
	depends = KAPOW_DEPS,
	libraries = ['z']
	)

setup(name = 'kapow',
	version = '0.1-%d' % SVN_REVISION,
	description = 'PyKAPOW, a python interface to kapow library.',
	maintainer = 'Sabi',
	maintainer_email = 'adeymo@dc.uba.ar',
	ext_modules = [kapow],
	py_modules = KAPOW_PY
)
