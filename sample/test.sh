#!/bin/sh

# clangの-O3ビルドは継続の処理周りで問題を抱えているかもしれない
clang -O3 ./main.c

./a.out "`cat ./sample/callcc.scm`"
./a.out "`cat ./sample/anonymous_recursion.scm`"
./a.out "`cat ./sample/counter.scm`"
./a.out "`cat ./sample/fizzbuzz.scm`"
./a.out "`cat ./sample/lambda.scm`"
./a.out "`cat ./sample/takeuchi.scm`"

gcc -O3 ./main.c

./a.out "`cat ./sample/callcc.scm`"
./a.out "`cat ./sample/anonymous_recursion.scm`"
./a.out "`cat ./sample/counter.scm`"
./a.out "`cat ./sample/fizzbuzz.scm`"
./a.out "`cat ./sample/lambda.scm`"
./a.out "`cat ./sample/takeuchi.scm`"

