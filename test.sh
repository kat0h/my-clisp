gcc -Wall --analyzer main.c

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

echo "括弧単体の出力"
./a.out '()'

echo;echo

echo "aは1→3 bは2"
./a.out '(begin (define a 1) (define b 2) (showenv) (set! a 3) (showenv))' 

echo;echo

echo "xを引数に取るlambda"
./a.out '(print (lambda (x) x))'
