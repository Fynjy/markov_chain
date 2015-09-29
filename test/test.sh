#!/bin/bash

cd "$(dirname "$0")"

OUTPUT_DIR=./output
URL_FILE=./files.txt
MODEL_FILE=$OUTPUT_DIR/model.txt
OUT_FILE=$OUTPUT_DIR/out.txt
ERROR_FILE=$OUTPUT_DIR/error.txt

rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

../markov_chain --action learn -i $URL_FILE -n 3 -o $MODEL_FILE >> $OUT_FILE 2>> $ERROR_FILE

if [ $? -ne 0 ]
then
  echo "Learn FAILED"
  exit 1
fi

../markov_chain --action predict -m $MODEL_FILE -c 10000 -i ./predict.txt >> $OUT_FILE 2>> $ERROR_FILE

if [ $? -ne 0 ]
then
  echo "Predict FAILED"
  exit 2
fi
if [ ! -s $ERROR_FILE ]
then
  echo "SUCCESS"
else
  echo "$ERROR_FILE isn't empty"
  exit 3
fi

exit 0