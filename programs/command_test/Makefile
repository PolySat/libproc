# Makefile for process command test program

C=gcc
CFLAGS=-c -Wall -pedantic -std=c99 -g
LDFLAGS=-lproc
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=proccmd

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	 $(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE)

