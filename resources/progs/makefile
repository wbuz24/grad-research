CC = gcc
CFLAGS = $(INCLUDES) -Wall -Wextra

EXECUTABLES = bin/arrflip

all: $(EXECUTABLES)

bin/arrflip: src/arrayflip.c
	$(CC) $(CFLAGS) -o bin/arrflip src/arrayflip.c

clean:
	rm $(EXECUTABLES)

