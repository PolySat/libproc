# Makefile for process command test program

C=gcc
CFLAGS=-Wall -pedantic -std=c99 -g
LDFLAGS=-lproc -ldl

all: $(SOURCES) multicast_src multicast_dst multi_src multi_sink

test_schema.h: test_schema.xp
	 poly-xdrgen --target libproc --output test_schema test_schema.xp

test_schema.c: test_schema.xp
	 poly-xdrgen --target libproc --output test_schema test_schema.xp

multi_sink: multi_sink.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

multi_src: multi_src.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

multicast_src: multicast_src.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# multicast_dst: multicast_dst.c
multicast_dst: multicast_dst.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf *.o multicast_src multicast_dst multi_sink multi_src

