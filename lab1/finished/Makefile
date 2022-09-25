#########################################################################
# ECE252 Lab Makefile
# Y. Huang, 2018/10/15
#########################################################################
all: concatpng findpng
concatpng:
	gcc -o concatpng.o concatpng.c -lz

findpng:
	gcc -o findpng.o findpng.c -lz

.PHONY: clean
clean:
	rm -f *.d *.o *.out