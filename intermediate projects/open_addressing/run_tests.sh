#!/bin/sh
# Usage: sh run_tests.sh your_file.c
[ -z "$1" ] && { echo "usage: sh $0 your_file.c"; exit 2; }
cc -Wall -Wextra -g -fsanitize=address,undefined -DOA_FILE="\"$1\"" \
   -o ./oa_tests_bin oa_tests.c 2>&1 | grep -E "error" && exit 2
./oa_tests_bin
