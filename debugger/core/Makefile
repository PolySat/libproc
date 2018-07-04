include Make.rules.arm

override LDFLAGS+=-lzmq -lproc -ldl -lm -lz
override CPPFLAGS+=-rdynamic -std=c++11 -Wall -Werror -g -fpic -D_GNU_SOURCE -D_FORTIFY_SOURCE=2

# Input/Output Variables
SOURCES=eventtimer.cc

# Private Variables
OBJECTS=$(SOURCES:.cc=.o)
LFLAGS=-shared -Wl,-soname,libprocdebug.so
SRC_PATH=.

all: $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(SO_LDFLAGS) $(OBJECTS) $(LDFLAGS) $(LFLAGS) -o libprocdebug.so

.PHONY: clean

.cc.o:
	$(CXX) $(CPPFLAGS) -c $(SRC_PATH)/$< -o $@

clean:
	rm -rf *.o *.so *.pb.*

