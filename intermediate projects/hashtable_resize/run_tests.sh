#!/bin/sh
# Usage: sh run_tests.sh your_file.c
[ -z "$1" ] && { echo "usage: sh $0 your_file.c"; exit 2; }
cc -Wall -Wextra -g -fsanitize=address,undefined -DHT_FILE="\"$1\"" \
   -o ./resize_tests_bin resize_tests.c 2>&1 | grep -E "error" && exit 2
./resize_tests_bin
