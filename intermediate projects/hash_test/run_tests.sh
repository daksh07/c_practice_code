#!/bin/sh
# Usage: ./run_tests.sh your_hashtable.c
[ -z "$1" ] && { echo "usage: $0 your_hashtable.c"; exit 2; }
gcc -Wall -Wextra -g -fsanitize=address,undefined -DHT_FILE="\"$1\"" \
    -o ./ht_tests_bin ht_tests.c 2>&1 | grep -E "error" && exit 2
./ht_tests_bin
