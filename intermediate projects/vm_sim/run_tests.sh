#!/bin/sh
# Usage: sh run_tests.sh your_vm.c
[ -z "$1" ] && { echo "usage: sh $0 your_vm.c"; exit 2; }
cc -Wall -Wextra -g -fsanitize=address,undefined -DVM_FILE="\"$1\"" \
   -o ./vm_tests_bin vm_tests.c 2>&1 | grep -E "error" && exit 2
./vm_tests_bin
