(begin (define c #f) (define i 0) (call/cc (lambda (cond) (set! c cond))) (set! i (+ i 1)) (print i) (if (< i 10) (c #f) ()))
