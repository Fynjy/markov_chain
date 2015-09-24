#!/bin/bash

g++ -O3 -Wall -c -fmessage-length=0 -std=c++11 -o main.o "main.cpp"
g++ -o markov_chain main.o -lpthread
