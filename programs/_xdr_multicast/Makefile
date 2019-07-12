# Makefile for process command test program

C=gcc
CFLAGS=-Wall -pedantic -std=c99 -g
LDFLAGS=-lproc3 -ldl

all: $(SOURCES) multi_src multi_dst #ex_r ex_t

multi_src: multi_src.c test_schema.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

multi_dst: multi_dst.c test_schema.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test_schema.h: test_schema.x
	 poly-xdrgen --target libproc --output test_schema test_schema.x

test_schema.c: test_schema.x
	 poly-xdrgen --target libproc --output test_schema test_schema.x


ex_r: ex_r.c
	 $(CC) $< -o $@

ex_t: ex_t.c
	 $(CC) $< -o $@

clean:
	rm -rf *.o multi_src multi_dst test_schema.c test_schema.h

