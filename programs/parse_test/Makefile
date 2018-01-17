# Makefile for example process

C=gcc
CFLAGS=-c -Wall -pedantic -std=c99 -g -I../..
LDFLAGS=
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=parser

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	 $(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE)

