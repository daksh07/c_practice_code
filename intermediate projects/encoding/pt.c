#include <stdint.h>
#include <stdio.h>

int main() {
  int a = 9;
  printf("Value of a: %d\n", a);
  int *b = &a;
  printf("Address of a: %d\n", b);
  *b = 21;
  printf("New value of a: %d\n", a);
  char c = 'A';
  printf("Byte value of A: %d\n", (uint8_t)c);
  const uint8_t input[] = "AAAABBBCCD";
  printf("Size of input: %lu\n", sizeof(input));
  uint8_t num_array[6] = {1, 3, 4, 6, 7, 4};
  printf("Size of int array (6 elements): %lu\n", sizeof(num_array));
}
