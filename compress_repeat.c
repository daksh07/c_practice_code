/* Problem: Run-length encoding compresses runs of repeated bytes into (value,
 * count) pairs. For example: Input: {5, 5, 5, 5, 2, 2, 9} (7 bytes) Output: {5,
 * 4, 2, 2, 9, 1} (6 bytes) — meaning "the value 5 repeated 4 times, then 2
 * repeated 2 times, then 9 repeated 1 time" Return value: the function should
 * return the number of bytes actually written into output (6, in this example)
 * — not a boolean, not the input length, the actual output size, since the
 * caller needs to know how much of output is meaningful. */

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

size_t rle_encode(const uint8_t *input, size_t input_len, uint8_t *output,
                  size_t output_max_len) {
  uint8_t index = 0;
  uint8_t count = 1;
  uint8_t last_byte = input[0];
  bool overflow = false;

  if (input_len == 0) {
    return 0;
  }

  //****** Main compress logic ******
  for (size_t i = 1; i < input_len; i++) {

    // If the next element is same simply increment the count
    if (last_byte == input[i] && count < 255) {
      count++;
    } else {
      if (index + 1 >= output_max_len) {
        overflow = true;
        break;
      }
      output[index] = last_byte;
      output[index + 1] = count;
      index += 2;
      last_byte = input[i];
      count = 1;
    }
  }

  if (!overflow) {
    if (index + 1 >= output_max_len) {
      overflow = true;
    } else {
      output[index] = last_byte;
      output[index + 1] = count;
      index += 2;
    }
  }

  if (overflow) {
    printf("ERROR! Output out of bound.\n");
    return 0;
  }

  for (size_t i = 0; i < output_max_len; i++) {
    printf("Output: %d\n", output[i]);
  }

  return index;
}

int main(void) {
  uint8_t output[6] = {0};
  uint8_t input[6] = {0, 0, 0, 1, 1, 3};
  rle_encode(input, 6, output, 6);
  return 0;
}
