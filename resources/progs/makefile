CC=gcc
GCC=g++
GEM5_FLAGS=-I/home/wbuziak/repos/gem5/include
GEM5_RISCV_FLAGS=-L/home/wbuziak/repos/gem5/util/m5/build/riscv/out
CFLAGS=$(INCLUDES) -Wall -Wextra -O0 

EXECUTABLES=bin/arrflip bin/sam-bench bin/analyze

all: $(EXECUTABLES)

bin/arrflip: src/arrayflip.c
	$(CC) $(GEM5_FLAGS) $(GEM5_RISCV_FLAGS) $(CFLAGS) -o bin/arrflip src/arrayflip.c

bin/sam-bench: src/sam-bench.cpp
	$(GCC) $(GEM5_FLAGS) $(GEM5_RISCV_FLAGS) $(CFLAGS) -o bin/sam-bench src/sam-bench.cpp

bin/analyze: src/analyzeStats.cpp
	$(GCC) $(CFLAGS) -o bin/analyze src/analyzeStats.cpp


clean:
	rm $(EXECUTABLES)

