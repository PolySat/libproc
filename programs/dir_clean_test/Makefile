# Makefile for example process

C=gcc
CFLAGS=-c -Wall -Werror -std=gnu99 -g -I../..
LDFLAGS=
SOURCES=main.c ../../util.c ../../priorityQueue.c ../../debug.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=dir_cleaner

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	 $(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
	 $(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE)

