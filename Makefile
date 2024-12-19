CFLAGS=-Wno-unused-result

all: lisp

lisp: lisp.c util.h
	gcc $(CFLAGS) lisp.c -o lisp

test: lisp
	./lisp test.scm -o out.s
	gcc driver.c out.s -o a.out
	./a.out
