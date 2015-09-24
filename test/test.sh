#!/bin/bash -x

../markov_chain --action learn -i files.txt -n 3 -o markov_chain.txt
../markov_chain --action predict -m markov_chain.txt -c 10000 -i input.txt
