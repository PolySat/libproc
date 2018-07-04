CFLAGS=-Wall -Werror -std=gnu99 -rdynamic -D_GNU_SOURCE -D_FORTIFY_SOURCE=2
LDFLAGS=-Wl,-rpath,/usr/local/lib -lproc -lprocdebug -ldl -lprotobuf

SRC=main.c
OBJS=$(SRC:.c=.o)

EXECUTABLE=main

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -fr *.o $(EXECUTABLE)
