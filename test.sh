gcc -Wall --analyzer main.c

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

echo '()'
./a.out '()'

echo '(begin (define a 1) (define b 2) (trace) (set! a 3) (trace))'
./a.out '(begin (define a 1) (define b 2) (trace) (set! a 3) (trace))' 
