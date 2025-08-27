CC=gcc
GCC=g++
CFLAGS+=-I/home/wbuziak/repos/gem5/include
FLAGS=$(INCLUDES) -Wall -Wextra -O0 

EXECUTABLES=bin/arrflip bin/sam-bench

all: $(EXECUTABLES)

bin/arrflip: src/arrayflip.c
	$(CC) $(CFLAGS) $(FLAGS) -o bin/arrflip src/arrayflip.c

bin/sam-bench: src/sam-bench.cpp
	$(GCC) $(CFLAGS) $(FLAGS) -o bin/sam-bench src/sam-bench.cpp

clean:
	rm $(EXECUTABLES)

