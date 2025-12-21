CC=gcc
GCC=g++
GEM5_FLAGS=-I/home/wbuziak/repos/gem5/include
GEM5_RISCV_FLAGS=-L/home/wbuziak/repos/gem5/util/m5/build/riscv/out
CFLAGS=$(INCLUDES) -Wall -Wextra -O0 

EXECUTABLES=bin/analyze bin/arrflip bin/sam-bench 

all: $(EXECUTABLES)

bin/analyze: src/analyzeStats.cpp
	$(GCC) $(CFLAGS) -o bin/analyze src/analyzeStats.cpp

bin/arrflip: src/arrayflip.c
	$(CC)  $(CFLAGS) -o bin/arrflip src/arrayflip.c

bin/sam-bench: src/sam-bench.cpp
	$(GCC) -o bin/sam-bench src/sam-bench.cpp

clean:
	rm $(EXECUTABLES)

