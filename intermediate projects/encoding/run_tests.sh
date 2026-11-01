#!/bin/sh
# Usage: sh run_tests.sh your_rle.c
[ -z "$1" ] && { echo "usage: sh $0 your_rle.c"; exit 2; }
cc -Wall -Wextra -g -fsanitize=address,undefined -DRLE_FILE="\"$1\"" \
   -o ./rle_tests_bin rle_tests.c 2>&1 | grep -E "error" && exit 2
./rle_tests_bin
