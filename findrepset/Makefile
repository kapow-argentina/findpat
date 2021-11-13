CFLAGS=-pedantic -std=c99 -Wall -Werror -O2
LDFLAGS=-lm
OPTFLAGS=-fprofile-use
CXX=g++
CC=gcc
LC_ALL=C
export LC_ALL

CFLAGS+= -Ialgo/ -Ilib/ -Icommon/ -I.
OBJS=


TOOLS=runcop filecop

FILES=bittree.o bwt.o lcp.o mmrs.o mrs.o output_callbacks.o cop.o enc.o common.o tiempos.o

BINS:=$(TOOLS:%=%)
OBJS+=$(FILES:%=%)
MAINOBJS:=$(BINS:%=%.o)
SRCS:=$(OBJS:.o=.c)
HEADS:=$(shell ls *.h)
MAINSRCS=$(MAINOBJS:.o=.c)

all: $(BINS)

runcop.o: $(HEADS) runcop.c
	$(CC) $(CFLAGS) -c runcop.c -o runcop.o


filecop.o: $(HEADS) filecop.c
	$(CC) $(CFLAGS) -c filecop.c -o filecop.o

runcop: $(OBJS) runcop.o
	$(CC) $(CFLAGS) $(LDFLAGS) runcop.o $(OBJS) -o runcop
	
filecop: $(OBJS) filecop.o
	$(CC) $(CFLAGS) $(LDFLAGS) filecop.o $(OBJS) -o filecop

clean:
	rm -rf $(OBJS) $(MAINOBJS) $(BINS) deps
