CFLAGS=-Wall -Werror -std=gnu99 -g
LDFLAGS=-rdynamic -lproc3 -ldl -lm -L /usr/local/lib

SRC=stopwatch.c
OBJS=$(SRC:.c=.o)

EXECUTABLE=stopwatch

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJS) $(LDFLAGS)
