# Makefile for process command test program

C=gcc
CFLAGS=-rdynamic -Wall -pedantic -std=c99 -g 
LDFLAGS= -lproc3 -ldl

all: test_proc test-util multi_listener

test_proc: test_proc.c test_schema.c
	 $(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

multi_listener: multi_listener.c test_schema.c
	 $(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test_schema.h: test_schema.x
	 poly-xdrgen --target libproc --output test_schema test_schema.x

test_schema.c: test_schema.x
	 poly-xdrgen --target libproc --output test_schema test_schema.x

test-util: test_schema.c test-util.c
	 $(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

util:
	 LD_LIBRARY_PATH=../.. ./test-util

test:
	 LD_LIBRARY_PATH=../.. ./test_proc

clean:
	rm -rf *.o test_proc test_schema.c test_schema.h

