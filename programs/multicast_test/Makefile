# Makefile for process command test program

C=gcc
CFLAGS=-Wall -pedantic -std=c99 -g
LDFLAGS=-lproc -ldl

all: $(SOURCES) multi_src multi_sink #ex_r ex_t

multi_src: multi_src.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

multi_sink: multi_sink.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

ex_r: ex_r.c
	 $(CC) $< -o $@

ex_t: ex_t.c
	 $(CC) $< -o $@

clean:
	rm -rf *.o multi_src multi_sink

