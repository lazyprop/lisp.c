CFLAGS=-Wno-unused-result

all: lisp

lisp: lisp.c util.h
	gcc $(CFLAGS) lisp.c -o lisp

asm: lisp
	./lisp > out.s
	gcc out.s
	./a.out
