all: highmtf lowcompl_right lowcomplmrs_left uncover dif

highmtf: highmtf.c
	gcc -o highmtf highmtf.c ../common/common.o -I../common/ -I../algo/ -I.. -lz -std=c99

##

lowcompl_right: lowcompl_right.c nsrph.o
	gcc -o lowcompl_right lowcompl_right.c nsrph.o ../common/common.o ../algo/*.o -I../common/ -I../algo/ -I.. -lz -lm -std=c99 -DGZIP

lowcomplmrs_left: lowcomplmrs_left.c nsrph.o
	gcc -o lowcomplmrs_left lowcomplmrs_left.c nsrph.o ../common/common.o ../algo/*.o -I../common/ -I../algo/ -I.. -lz -lm -std=c99 -DGZIP

uncover: uncover.c nsrph.o
	gcc -o uncover uncover.c nsrph.o ../common/common.o ../algo/*.o -I../common/ -I../algo/ -I.. -lz -lm -std=c99 -DGZIP

dif: dif.c nsrph.o
	gcc -o dif dif.c nsrph.o ../common/common.o ../algo/*.o -I../common/ -I../algo/ -I.. -lz -lm -std=c99 -DGZIP

nsrph.o: nsrph.c
	gcc -c -o nsrph.o nsrph.c -I../common/ -I../algo/ -I.. -std=c99 -DGZIP

