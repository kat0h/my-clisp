gcc -Wall --analyzer main.c

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

echo '=> ()'
./a.out '()'

echo;echo

echo "Env"
echo "  ((a 1.000000) (b 2.000000))"
echo "Env"
echo "  ((a 3.000000) (b 2.000000))"
echo "=> 0.000000"
./a.out '(begin (define a 1) (define b 2) (trace) (set! a 3) (trace))' 

echo;echo

echo "=> Closure"
echo "Arguments:"
echo "  (x)"
echo "Body:"
echo "  (x)"
echo "Env"
echo "  ()"
./a.out '(lambda (x) x)'
