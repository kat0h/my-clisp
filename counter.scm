(begin 
  (define newcounter 
    (lambda ()
      ((lambda (x)
         (lambda ()
           (begin
             (set! x (+ x 1))
             (print x)
             )))
       0)))
  (define counter1 (newcounter))
  (define counter2 (newcounter))
  (counter1)
  (counter2)
  (counter2)
  (counter1)
  (counter1))

