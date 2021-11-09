#!/usr/bin/env python
import random
import subprocess

def main():
	"""	Runs runbwt and runobwt comparing their outputs for all the strings
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
	rand_lengths = [31, 32, 33, 36, 37, 40, 100, 101, 128, 1023, 1024, 1200, \
					4096, 4095, 4097, 4800]
	alph_sizes = [1, 2, 3, 5, 7, 10]
	times = 1
	rand_strings = []
	for s in alph_sizes:
		for l in rand_lengths:
			for t in range(times):
				st = []
				freq = l*2 / s + 1
				for i in range(s):
					st += list(str(i) * freq)
				random.shuffle(st)
				st = st[:l]
				st = "".join(st)
				rand_strings.append(st)
		
	strings = det_strings + rand_strings
	l = len(strings)

	err = 0
	suc = 0

	for i in range(l):
#		print "String", i, ":", strings[i], "(", len(strings[i]), ")"
		filename = "tmp" + str(i)
		filenameb = "tmpbf" + str(i)
		f = open(filename, "w+")
		fb = open(filenameb, "w+")
		subprocess.call(["./runbwt", strings[i]], stdout = f)
		subprocess.call(["./runobwt", strings[i]], stdout = fb)
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
