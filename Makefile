all: example

example: example.cpp
	g++ -std=c++11 -pthread -g -o $@ example.cpp

clean:
	rm example