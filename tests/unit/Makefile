GTEST_DIR = ./googletest/googletest

CODEDIR = ../..
EXECUTABLE = tests

# Where to find user code.
CPPFLAGS += -isystem $(GTEST_DIR)/include -std=c++11
CXXFLAGS += -g -ldl -pthread

TESTS = test_events.cc test_virtclk.cc
OBJECTS=$(TESTS:.cc=.o)

GTEST_HEADERS := $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

all : googletest code $(OBJECTS) gtest_main.a 
	$(CXX) $(wildcard $(CODEDIR)/*.o) $(OBJECTS) gtest_main.a $(CPPFLAGS) $(CXXFLAGS) -o $(EXECUTABLE)

code :
	make -C $(CODEDIR)

googletest:
	git clone git@asof.atl.calpoly.edu:libraries/googletest

clean :
	rm -fr googletest gtest.a gtest_main.a *.o $(EXECUTABLE)

gtest-all.o :
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o :
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

.cc.o:
	 $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
