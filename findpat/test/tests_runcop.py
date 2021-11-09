#!/usr/bin/env python
import random
import subprocess
import sys

def main():
	"""	Runs runcop and runcop_bruteforce comparing their outputs for all the strings
defined below and random strings with alphabet sizes 'alph_sizes' and lengths
'rand_lengths' ('times' tests for each combination). 
	Because it generates all the outputs this test is not suitable for time
benchmarks."""
	det_strings = ["x","xx","xy","yx","mississipi","abcdabcdabcdabcd", \
		"abcdabcdabcdabcdefgh","aaaaaaaaaaaaaaa","aaaaaaaaaaaab","baaaaaaaa", \
		"abababababab","aaaaabaaaaabaaaaabaaaaab", \
		"abcdefghijklmnopqrstuvwxyz", \
		"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz", \
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", \
		"aba","aab","baa","abc","bca","acb","bac","cab","cba", \
		"abaaba","aabaab","baabaa","abcabc","bcabca","acbacb","bacbac", \
		"cabcab","cbacba", \
		"abaabaaba","aabaabaabaab","baabaabaabaa","abcabcabcabc", \
		"bcabcabcabca","acbacbacbacb","bacbacbacbacbac","cabcabcabcab", \
		"cbacbacbacba","abaabaaba","aabaabaabaaba","baabaabaabaaba", \
		"abcabcabcabca","bcabcabcabcab","acbacbacbacbac","bacbacbacbacbacb", \
		"cabcabcabcabc","cbacbacbacbac", \
		"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyza", \
		"zxywvutsrqponmlkjihgfedcbazxywvutsrqponmlkjihgfedcba", \
		"zxywvutsrqponmlkjihgfedcbaabcdefghijklmnopqrstuvwxyz", \
		"unkcvxyvwfxerpwhgwwonatogeuyaouudgwbdwwb", \
		"cdccdcbddbcbcbdbaecccdcdddcbcdeac", \
		"rrrllleee", "rrrrlllleeee", "rrrrrleeeee", \
		"a" * 50, 	"a" * 49 + "bbbbccccde", "a" * 64, "abc" * 31 ]

	det_cases = len(det_strings)
	rand_lengths = [1, 2, 3, 5, 7, 10, 16, 
					31, 32, 33, 36, 37, 40] 
				#	100, 101, 128, 
				#	1023, 1024, 1200, 
				#	4096, 4095, 4097, 4800]
	alph_sizes = [1, 2, 3, 5, 7, 10]
	times = 1
	rand_strings = []
	for s in alph_sizes:
		fixed_size_strings = []
		for l in rand_lengths:
			for t in range(times):
				st = []
				freq = l*2 / s + 1
				for i in range(s):
					st += list(str(i) * freq)
				random.shuffle(st)
				st = st[:l]
				st = "".join(st)
				fixed_size_strings.append(st)
		rand_strings.append(fixed_size_strings)
		
	
	#DEBUG
#	strings = [rand_strings[2]]
	strings = [det_strings] + rand_strings
	l = len(strings)

	err = 0
	suc = 0
	
	for i in range(l):

		times = len(strings[i])
		for j in range(times):
			if '-c' in sys.argv:
				parameters = [random.choice(strings[i]) 
								for k in range(random.randint(2,7))] + ["-c"]
								# DEBUG
			#					for k in range(2)] + ["-c"]
			else:
				parameters = [random.choice(strings[i]) 
								for k in range(random.randint(2,7))]
								# DEBUG
			#					for k in range(2)]
			filename = "tmp" + str(i) + '-' + str(j)
			filenameb = "tmpbf" + str(i) + '-' + str(j)
			f = open(filename, "w+")
			fb = open(filenameb, "w+")

			print "Testing strings:", ' '.join(parameters[:-1])

			subprocess.call(["./runcop"] + parameters, stdout = f)
			subprocess.call(["./runcop_bruteforce"] + parameters, stdout = fb)

			f.seek(0)
			fb.seek(0)

			fl = f.readlines()
			fbl = fb.readlines()

			fl.sort()
			fbl.sort()

			f.writelines(fl)
			fb.writelines(fbl)
			
			# TODO: sort lines
			f.close()
			fb.close()
	
			ret = subprocess.call(["diff", filename, filenameb])
			if ret:
				err += 1
	#			print i, ":", strings[i], "ERROR"
				print i, ": ERROR"
			else:
				suc += 1
	#			print i, ":", strings[i], "OK"
				print i, ": OK"
	
			subprocess.call(["rm", filename, filenameb])
	print "--- Tests finished --- "
	print "Results:", err, "errors,", suc, "tests OK."

main()
