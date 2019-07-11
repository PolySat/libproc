# Makefile for process command test program

C=gcc
CFLAGS=-Wall -pedantic -std=c99 -g
LDFLAGS=-lproc -ldl

all: $(SOURCES) multicast_src multicast_dst ex_r ex_t

multicast_src: multicast_src.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

multicast_dst: multicast_dst.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

ex_r: ex_r.c
	 $(CC) $< -o $@

ex_t: ex_t.c
	 $(CC) $< -o $@

clean:
	rm -rf *.o multicast_src multicast_dst

