all: test cube.so foo.so sat.so
	 cp -f cube.so plugs/test1-cube.so
	 cp -f sat.so plugs/cube-sat.so
	 cp -f foo.so foo/bar/preload-test.so
	 cp -f foo.so plugs/preload-foo.so

test: test.c
	 $(CC) -g -rdynamic -Wall test.c -lproc3 -o test

foo.so: foo.c
	 $(CC) -g -Wall foo.c -shared -fPIC -Wl,-soname,foo.so -o foo.so

cube.so: cube.c
	 $(CC) -g -Wall cube.c -shared -fPIC -Wl,-soname,cube.so -o cube.so

sat.so: sat.c
	 $(CC) -g -Wall sat.c -shared -fPIC -Wl,-soname,sat.so -o sat.so

clean:
	 rm -f test foo.so cube.so
