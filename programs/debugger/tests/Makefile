CFLAGS=-I../../.. -Wall -Werror -std=gnu99 -rdynamic -D_GNU_SOURCE -D_FORTIFY_SOURCE=2
LDFLAGS=-L../../.. -Wl,-rpath,/usr/local/lib -lproc -ldl

SRC=main.c
OBJS=$(SRC:.c=.o)

EXECUTABLE=main

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -fr *.o $(EXECUTABLE)

run: $(EXECUTABLE)
	LD_LIBRARY_PATH=../../.. ./$(EXECUTABLE)

dbg: $(EXECUTABLE)
	LD_LIBRARY_PATH=../../.. gdb ./$(EXECUTABLE)
