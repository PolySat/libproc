# Makefile for process command test program

C=gcc
CFLAGS=-Wall -pedantic -std=c99 -g
LDFLAGS=-lproc -ldl

all: $(SOURCES) test

test: test.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf *.o test

