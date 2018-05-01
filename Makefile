all: singlepath_str test

singlepath_str: mjpath.c mjpath.h singlepath_str.c
	gcc -Wall -Wextra -g -o $@  mjpath.c singlepath_str.c 

bin/test: build/mjpath.o build/test.o
	gcc -Wall -Wextra -o $@ $^

bin:
	mkdir -p $@
build:
	mkdir -p $@

build/mjpath.o: mjpath.h

build/%.o: test/%.c build
	gcc -c -Wall -Wextra -g -o $@ $<

build/%.o: %.c build
	gcc -c -Wall -Wextra -g -o $@ $<
