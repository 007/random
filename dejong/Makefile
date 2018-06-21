default: run

dejong: main.cpp
	gcc -DTHREAD_COUNT=6 -pthread -Wall -Wpedantic -Werror main.cpp -Ofast -march=native -mtune=native -o dejong

run: dejong
	./dejong
