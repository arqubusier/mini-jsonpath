all: singlepath_str test

singlepath_str: mjpath.c mjpath.h singlepath_str.c
	gcc -Wall -Wextra -g -o $@  mjpath.c singlepath_str.c 

test: mjpath.c mjpath.h test.c
	gcc -Wall -Wextra -g -o $@  mjpath.c test.c 
