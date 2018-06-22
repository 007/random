default: run

dejong: main.cpp
	g++ -DTHREAD_COUNT=6 -std=c++14 -pthread -Wall -Wpedantic -Werror main.cpp -Ofast -march=native -mtune=native -o dejong

check: dejong
	clang-tidy main.cpp -checks='readability-braces-around-statements' -- -std=c++14

run: dejong check
	./dejong
