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
