# Makefile for process command test program

C=gcc
CFLAGS=-rdynamic -Wall -pedantic -std=c99 -g -I../..
LDFLAGS=-L../.. -lproc -ldl

all: zmq_server cpp_client dbg_client dbg_target

zmq_server: zmq_server.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

dbg_target: dbg_target.c
	 $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

cpp_client: cpp_client.cpp
	 $(CXX) $(CXXFLAGS) $< -o $@ -lzmq

dbg_client: dbg_client.cpp
	 $(CXX) $(CXXFLAGS) $< -o $@ -lzmq

run:
	 LD_LIBRARY_PATH=../.. ./zmq_server

runc:
	 LD_LIBRARY_PATH=../.. ./dbg_target

dbg:
	 LD_LIBRARY_PATH=../.. gdb ./zmq_server

clean:
	rm -rf *.o zmq_server dbg_target cpp_client dbg_client

