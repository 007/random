default: dejong

dejong: main.cpp
	gcc -DTHREAD_COUNT=6 -pthread -Wall -Wpedantic -Werror main.cpp -Os -march=native -mtune=native -o dejong
