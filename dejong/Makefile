default: run

dejong: main.cpp
	g++ -DTHREAD_COUNT=6 -pthread -Wall -Wpedantic -Werror main.cpp -Ofast -march=native -mtune=native -o dejong

run: dejong
	./dejong
