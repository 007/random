default: run

dejong: main.cpp
	g++ -DTHREAD_COUNT=6 -std=c++14 -pthread -Wall -Wpedantic -Werror main.cpp -Ofast -march=native -mtune=native -o dejong

CLANG_CHECKS:="readability-implicit-bool-conversion,performance-type-promotion-in-math-fn,misc-unused-parameters,google-runtime-int,hicpp-deprecated-headers,llvm-include-order,misc-macro-parentheses,modernize-use-nullptr,modernize-deprecated-headers,readability-braces-around-statements"
check: dejong
	clang-tidy main.cpp -checks='$(CLANG_CHECKS)' -- -std=c++14

fixcheck: dejong
	clang-tidy -fix main.cpp -checks='$(CLANG_CHECKS)' -- -std=c++14

run: dejong check
	./dejong
