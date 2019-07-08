# Makefile for process command test program

CC=gcc
CFLAGS=-c -Wall -pedantic -std=c99 -g
LDFLAGS=-lproc
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=proccmd

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) -Wall $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE)

re:
	make clean
	make
