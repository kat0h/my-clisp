(define fizzbuzz
  (lambda (n i)
    (begin
      (if (= (+ n 1) i)
        ()
        (begin
          (fb i)
          (fizzbuzz n (+ i 1)))))))
(define fb
  (lambda (n)
    (begin
      (if (and (= (modulo n 3) 0) (= (modulo n 5) 0))
        (print "FizzBuzz")
        (if (= (modulo n 3) 0)
          (print "Fizz")
          (if (= (modulo n 5) 0) (print "Buzz") (print n)))))))
(fizzbuzz 100 1)
