all: build

build: main.o
	g++ -o markov_chain main.o -lpthread -lboost_locale -lboost_program_options -lboost_iostreams

main.o: main.cpp CurlDevice.h MarkovChain.h Transitions.h WordIterator.h
	g++ -O3 -Wall -c -std=c++11 -o main.o main.cpp

clean:
	rm -rf ./main.o ./markov_chain

check:
	./test/test.sh

