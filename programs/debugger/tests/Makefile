CFLAGS=-I../../.. -Wall -Werror -std=gnu99 -rdynamic -D_GNU_SOURCE -D_FORTIFY_SOURCE=2
LDFLAGS=-L../../.. -Wl,-rpath,/usr/local/lib -lproc -ldl

SRC=main.c
OBJS=$(SRC:.c=.o)

EXECUTABLE=main

all: $(OBJS) main-util
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJS) $(LDFLAGS)

main-util: main-util.o
	$(CC) -o $@ $< $(LDFLAGS)


.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -fr *.o $(EXECUTABLE)

run: $(EXECUTABLE)
	LD_LIBRARY_PATH=../../.. ./$(EXECUTABLE)

status: main-util
	LD_LIBRARY_PATH=../../.. ./main-util

dbg: $(EXECUTABLE)
	LD_LIBRARY_PATH=../../.. gdb ./$(EXECUTABLE)
