#!/bin/bash

COUNTER=0;
while [ $COUNTER -lt 10 ]; do
  cd build
  rm tests/filesys/base/syn-read.*
  make tests/filesys/base/syn-read.result
  cat tests/filesys/base/syn-read.result >> results
  cat tests/filesys/base/syn-read.output >> output
  cat tests/filesys/base/syn-read.errors >> errors
  let COUNTER=COUNTER+1
  cd ..
done
