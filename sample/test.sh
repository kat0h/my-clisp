#!/bin/sh

make

./scm "`cat ./sample/callcc.scm`"
./scm "`cat ./sample/anonymous_recursion.scm`"
./scm "`cat ./sample/counter.scm`"
./scm "`cat ./sample/fizzbuzz.scm`"
./scm "`cat ./sample/lambda.scm`"
./scm "`cat ./sample/takeuchi.scm`"
./scm "`cat ./sample/generator.scm`"
./scm "`cat ./sample/corutine.scm`"
