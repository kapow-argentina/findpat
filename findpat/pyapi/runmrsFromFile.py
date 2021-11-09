# coding: utf-8

import sys

import kapow
import ktools


#def run_mrs(filename, sep=None):
def run_mrs(seq, sep, minLength):
	src = kapow.Array()
	
	src.bindFrom(seq+'\0')
        
	r = kapow.Array()
	p = kapow.Array()
	kapow.bwt(src, p=p, r=r)
	h = kapow.Array()
	kapow.lcp(src, h, p=p, r=r, sep=sep)

	### Callback function: could be a python callable
	cbk = kapow.Callback(kapow.CALLBACK_READABLE, (sys.stdout, src, r))

	## Call mrs
	kapow.mrs(src, r, h, p, ml=minLength, cback=cbk)


if __name__ == "__main__":
        fo = open(sys.argv[1],"r")
        seq = fo.read();
        fo.close();
        sep = sys.argv[2][0]  # el primer char del 2do parametro
        minLength=int(sys.argv[3])
        run_mrs(seq, sep, minLength)
