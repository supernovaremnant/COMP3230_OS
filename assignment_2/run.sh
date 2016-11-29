#!/bin/bash

make clean
make
echo "mode 0"
./agent 10 0

echo "mode 1"
./agent 10 1

echo "mode 2"
./agent 10 2
