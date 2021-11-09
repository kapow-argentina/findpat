# coding: utf-8

import sys, os, gzip, bz2, struct, socket, re
import kapow

class KapowPPZError(Exception):
	pass

STATS_FIELDS = [
	('I', 'rev'),				# source revision number of the code that generates this stats
	('I', 'long_source_a'),		# length of input file
	('I', 'long_source_b'),
	('I', 'long_input_a'),		# length of preprocessed input file (without Ns, before BWT)
	('I', 'long_input_b'),
	('I', 'n'),
	('I', 'nlzw'),
	('I', 'zero_mtf'),			# how many 0s in the output of BWT+MTF
	('I', 'nzz_est'),			# how many non-0 followed by 0 are in the output of BWT+MTF
	('I', 'long_output'),		# length (in number of codes) of BWT+MTF+LZW 
	('I', 'est_size'),			# estimated size of BWT+MTF+LZW+ARIT_ENC (in bytes)
	('I', 'mtf_entropy'),		# estimated size of BWT+MTF+ARIT_ENC (in bytes)
	('I', 'huf_len'),			# long of huffman table (in bits)
	('I', 'huf_tbl_bit'),
	('I', 'lzw_codes'),
	('Q', 'lpat_size'),			# estimated size for longpat, for ch=8, and ch=3, (in bits)
	('Q', 'lpat3_size'),
	('128s', 'filename_a'),
	('128s', 'filename_b'),
	('128s', 'hostname'),
	('I', 'cnt_A'),				# count how many ACGTN are in the real input
	('I', 'cnt_C'),
	('I', 'cnt_G'),
	('I', 'cnt_T'),
	('I', 'cnt_N'),
	('I', 'N_blocks'),			# how many blocks of more than 5 N are there
	# time stats in ms (wraps in 49 days)
	('I', 'time_bwt'),
	('I', 'time_mtf'),
	('I', 'time_lzw'),
	('I', 'time_mrs'),
	('I', 'time_huf'),
	('I', 'time_longp'),
	('I', 'time_tot'),
	# new params
	('Q', 'lpat3dl0_size'),
	('d', 'kinv_lcp'),
	('I', 'time_mmrs'),
]

class Section:
	def __init__(self, kpw):
		self.kpw = kpw
		self.could_load = False
	def should_save(self):
		return not self.could_load

class SectionSTATS(Section):
	"""Represents the STATS section of a KPW file. Public members: data"""
	def __init__(self, *args, **kwargs):
		Section.__init__(self, *args, **kwargs)
		self.field_names = map(lambda x: x[1], STATS_FIELDS)
		self.struct_fmt = '<' + ''.join(map(lambda x: x[0], STATS_FIELDS))
	def load(self):
		stats_data = self.kpw.load_stats()
		if stats_data == None:
			self._init_from('\x00' * struct.calcsize(self.struct_fmt))
			return False
		else:
			self.could_load = True
			self._init_from(str(stats_data))
			self._original_data = dict(self.data) # make a copy
			return True
	def save(self):
		field_values = struct.pack(self.struct_fmt, *map(lambda fn: self.data[fn], self.field_names))
		arr = kapow.Array()
		arr.bindFrom(field_values)
		self.kpw.save_stats(arr)
	def should_save(self):
		# should save if it was modified
		return not self.could_load or self.data != self._original_data
	def _init_from(self, sdata):
		field_values = struct.unpack(self.struct_fmt, sdata)
		self.data = dict(zip(self.field_names, field_values))
		for k in ['filename_a', 'filename_b', 'hostname']:
			self.data[k] = self.data[k].strip('\x00')

class SectionBWT(Section):
	"""Represents the BWT section of a KPW file. Public members: obwt, prim, s, p, r, h"""
	def load(self):
		self.s = self.p = self.r = self.h = None
		obwt_prim = self.kpw.load_bwt()
		if obwt_prim == None:
			return False
		else:
			self.could_load = True
			self.obwt, self.prim = obwt_prim
			return True
	def save(self):
		self.kpw.save_bwt(self.obwt, self.prim)
	def build_pr(self):
		self.p = kapow.Array()
		self.r = kapow.Array()
		kapow.bwt_pr(self.obwt, self.prim, self.p, self.r)
	def should_save(self):
		return not self.could_load and self.obwt and self.prim

class SectionTRAC(Section):
	"""Represents the TRAC section of a KPW file. Public members: trac_array, trac_middle"""
	def load(self):
		arr_mid = self.kpw.load_trac()
		if arr_mid == None:
			return False
		else:
			self.could_load = True
			self.trac_array, self.trac_middle = arr_mid
			return True
	def save(self):
		self.kpw.save_trac(self.trac_array, self.trac_middle)

class PPZ:
	def __init__(self, kpwfn=None, files=[], sep='\000', trac_positions=True):
		self.kpw = None
		self.kpwfn = kpwfn
		self.in_files = files
		if len(self.in_files) > 0:
			if len(self.in_files) > 2:
				raise KapowPPZError("PPZ cannot handle more than two files.")
			elif len(self.in_files) == 1:
				self.in_files.append('/dev/null')
			if not self.kpwfn:
				self.kpwfn = kpwFilename(*self.in_files)
		self.kpw = kapow.KPWFile(self.kpwfn)

		self.sections = {}
		self.options = {
			'sep': sep,
			'trac_positions': trac_positions,
		}

		self.init_stats()

	def init_stats(self):
		self.sections['STATS'] = sec = SectionSTATS(self.kpw)
		if not sec.load():
			if len(self.in_files) == 0:
				raise KapowPPZError("Should give at least one input file to create KPW file")
			sec.data['rev'] = 666 # FIXME: revision number
			sec.data['hostname'] = socket.gethostname()
			sec.data['filename_a'] = os.path.basename(self.in_files[0])
			sec.data['filename_b'] = os.path.basename(self.in_files[1])
		self.data = sec.data
		return sec

	def load_files(self, files):
		a = loadFasta(files[0], sep=self.options['sep'])
		b = loadFasta(files[1], sep=self.options['sep'])
		self.data['long_source_a'] = self.data['long_input_a'] = a.size
		self.data['long_source_b'] = self.data['long_input_b'] = b.size
		if self.options['trac_positions']:
			raise KapowPPZError('Error: cannot trac positions')
			trac = kapow.Array()
			a, k1 = kapow.fa_strip_n_bind(a, trac, 0)
			b, k2 = kapow.fa_strip_n_bind(b, trac, k1)
			self.data['long_input_a'] = a.size
			self.data['long_input_b'] = b.size
			self.sections['TRAC'] = sec = SectionTRAC(self.kpw)
			sec.trac_array = trac
			sec.trac_middle = kapow.trac_real_to_virt(trac, a.size + 1)
		n = a.size + b.size + 2
		s = kapow.Array(n)
		s.memcpy(a, 0, 0, a.size)
		s[a.size] = 0xfe
		s.memcpy(b, a.size + 1, 0, b.size)
		s[n - 1] = 0xff
		return s

	def bwt(self, build_obwt=True, build_pr=False):
		if 'BWT' in self.sections:
			sec = self.sections['BWT']
			if build_pr: sec.build_pr()
			return sec

		self.sections['BWT'] = sec = SectionBWT(self.kpw)
		if sec.load():
			if build_pr: sec.build_pr()
			return sec
		# Otherwise, calculate it
		sec.s = self.load_files(self.in_files)
		self.data['n'] = n = sec.s.size
		sec.obwt = kapow.Array(n, 1) if build_obwt else None
		sec.p = kapow.Array(n, 4) if build_pr else None
		sec.r = kapow.Array(n, 4) if build_pr else None
		sec.prim, time_bwt = kapow.bwt(sec.s, sec.obwt, sec.p, sec.r)
		self.data['time_bwt'] = int(time_bwt)
		self.data['time_tot'] += int(time_bwt)
		return sec

	def mtf(self):
		section_bwt = self.bwt()
		self.data['zero_mtf'], time_mtf = res = kapow.mtf(section_bwt.obwt)
		self.data['time_mtf'] = int(time_mtf)
		self.data['time_tot'] += int(time_mtf)
		return res

	def lcp(self):
		sec = self.bwt(build_pr=True)
		sec.h = kapow.Array()
		time_lcp = kapow.lcp(sec.s, sec.h, sec.p, sec.r)
		sys.stderr.write('Warning: no field in STATS for time_lcp: %f\n' % (time_lcp,))
		self.data['time_tot'] += int(time_lcp)
		return sec.h, time_lcp

	def klcp(self):
		h = self.lcp()
		self.data['kinv_lcp'], time_klcp = res = kapow.klcp(h)
		sys.stderr.write('Warning: no field in STATS for time_klcp: %f\n' % (time_klcp,))
		self.data['time_tot'] += int(time_klcp)
		return res

	def save(self):
		for sec in self.sections.values():
			if sec.should_save():
				sec.save()

# Usefull functions

def openFile(fn):
	if fn[-3:].lower() == '.gz' and os.path.exists(fn):
		f = gzip.open(fn, 'rb')
	elif fn[-4:].lower() == '.bz2' and os.path.exists(fn):
		f = bz2.open(fn, 'rb')
	elif os.path.exists(fn):
		f = open(fn, 'rb')
	elif os.path.exists(fn+'.gz'):
		f = gzip.open(fn+'.gz', 'rb')
	elif os.path.exists(fn+'.bz2'):
		f = bz2.open(fn+'.bz2', 'rb')
	else:
		raise IOError("File '%s' or compressed version not found." % fn)
	return f

def loadFile(fn):
	"""Returns a kapow.Array() with the contents of the file, that could be compressed."""
	f = openFile(fn)
	res = kapow.Array()
	res.bindFrom(f.read())
	return res

def loadFasta(fa, sep=None, term=None, nfilter=None):
	"""Returns a kapow.Array() with the contents of the file interpreted as a FASTA file, using sep if given."""
	def _boundary2int(num, n, start=False):
		if num == '': return 0 if start else n
		mult = 1
		if num[-1] == 'K': mult = 1000
		elif num[-1] == 'M': mult = 1000000
		elif num[-1] == 'G': mult = 1000000000
		elif num[-1] == 'T': mult = 1000000000000
		else: num = num + '_'
		num = int(num[:-1]) * mult
		if num < 0: return n + num
		return num
		
	m = re.match(r'(.*)\[([0-9]*[TGMK]?):([0-9]*[TGMK]?)\]', fa)
	if m:
		fa, left, right = m.groups()
		
	res = loadFile(fa)
	ores = kapow.fa_copy_cont_bind(res, sep)
	
	# Filters the long lists of Ns
	if nfilter:
		ores, lres = kapow.fa_strip_n_bind(ores)

	# Binds the desired portion
	if m:
		ores = ores[_boundary2int(left, len(ores), True):_boundary2int(right, len(ores), False)]
	
	if term != None:
		if ores.size < res.size:
			ores = res[0:ores.size+1]
		else:
			ores = kapow.Array(res.size+1,1)
			ores.memcpy(res)
		del res
		ores[ores.size-1] = term
	return ores

def saveFasta(fn, arr):
	"""Stores a kapow.Array() in a FASTA file."""
	f = open(fn, 'w')
	f.write('> ' + fn + '\n')
	for i in range(0,len(arr),80):
		f.write(str(arr[i:i+80]))
		f.write('\n')

def loadFastaTrac(fa, sep=None, term=None):
	"""Loads the FASTA filename 'fa' interpreting its content and bypass the
character 'sep' if given. Also convert the N's and returning trac array."""
	res = loadFasta(fa, sep, term)
	trac = kapow.Array()
	res,k = kapow.fa_strip_n_bind(res, trac)
	return res,trac

def karray_concat(arrs, sep=None, term=None):
	n = len(arrs)
	m = sum(x.size for x in arrs) + (n-1 if sep else 0) + (1 if term else 0)
	res = kapow.Array(m,1)
	i = 0
	for x in arrs:
		res.memcpy(x, dst=i)
		i += x.size
		if sep and i < m:
			res[i] = sep
			i += 1
	if term:
		res[m-1] = term
	return res

def loadManyFasta(files=[], sep=None, term=None):
	parts = [loadFasta(fn) for fn in files]
	return karray_concat(parts, sep, term)

def loadManyFastaTrac(files=[], sep=None):
	trac = kapow.Array()
	parts = []
	lres = 0
	for fn in files:
		s = loadFasta(fn)
		s, lr = kapow.fa_strip_n_bind(s, trac, lres)
		lres += lr
		#print 'lres =', lres, ' | trac =',trac
		if sep: lres += 1
		parts.append(s)
		del s
	res = karray_concat(parts, sep, sep)
	return res,trac

def strip_suffix(x, suf):
	if len(x) >= len(suf) and x[-len(suf):] == suf:
		 x = x[:-len(suf)]
	return x

def kpwFilename(file1, file2='/dev/null'):
	f1 = strip_suffix(os.path.basename(file1), '.gz')
	f2 = strip_suffix(os.path.basename(file2), '.gz')
	return '%s-%s.kpw' % (f1, f2)

