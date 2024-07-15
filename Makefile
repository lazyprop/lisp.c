CFLAGS=-Wno-unused-result

all: lisp

lisp: lisp.c util.h
	gcc $(CFLAGS) lisp.c -o lisp
