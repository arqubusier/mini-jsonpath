all: singlepath_str test

bin/eval: build/mjpath.o build/eval.o
	gcc -Wall -Wextra -o $@ $^

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
