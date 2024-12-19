# lisp.c

A small Lisp to x86_64 compiler written in C. Can compile basic programs: arithmetic, conditionals,
recursive defintions. Doesn't have any of the nice functional features yet.


Usage:
```
./lisp input.scm -o out.s
gcc out.s -o a.out
./a.out
```

Note: The `main` of the lisp program must be named `scheme_entry` since it links with `driver.c`.
Will make it standalone later.

Example:
(define (factorial n)
  (if (= n 0)
      1
      (* n (factorial (- n 1)))))

(define (fib n)
  (if (< n 2)
      n
      (+ (fib (- n 1)) (fib (- n 2)))))

(define (scheme_entry)
  (+ (fib 7) (factorial 5)) )
