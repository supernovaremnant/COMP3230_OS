#!/bin/bash

make clean
make

tmp="...................."

echo "$tmp"
echo "mode 0"
./agent 10 0
echo "$tmp"

echo "mode 1"
./agent 10 1
echo $tmp

echo "mode 2 (bonus)"
./agent 10 2
echo $tmp

