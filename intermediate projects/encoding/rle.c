#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Run-length encoding (RLE) of raw bytes.
 *
 * A "run" is a repeated byte. Instead of storing it again and again, we store
 * how many times it repeats and the byte itself:
 *
 *   input:   A A A A B B B C C D
 *   output:  4 'A'  3 'B'  2 'C'  1 'D'      (8 bytes instead of 10)
 *
 * Encoded format: a sequence of 2-byte pairs  [count][value]
 *   - count is 1..255 (0 is never valid)
 *   - a run longer than 255 is written as several pairs, e.g. 300 x 'A'
 *     becomes [255]['A'] [45]['A']
 *   - the data is arbitrary bytes. 0x00 is a normal value, not a terminator.
 *
 * Worst case (no repeats at all) the output is twice the size of the input.
 * Neither function may write past the end of `out` (out_cap bytes).
 */

typedef enum {
  RLE_OK,
  RLE_ERR_BUFFER_TOO_SMALL, // out_cap is not enough for the result
  RLE_ERR_MALFORMED,        // decode only: input is not valid encoded data
} rle_status_t;

#define RLE_MAX_ENCODED_SIZE(n) ((n) * 2)

void print_encoding(uint8_t *out, size_t *out_len, bool e) {
  if (e == 1) {
    size_t index = *out_len;
    printf("Encoded data: \n");
    for (size_t i = 0; i < index; i += 2) {
      printf("%d : %c\n", out[i], out[i + 1]);
    }
  } else {
    printf("Decoded data: \n");
    printf("%s\n", out);
  }
}
// Compress in[0..in_len) into out. On RLE_OK, *out_len is the bytes written.
// split a count of more tha 255. eg 270 x A -> 255, A, 15, A
// So for that rep_in = how many time 255 needs to the count for a very large
// repeat. So if count = 515, then rep_in = 2, hence ecoding -> 255, A, 255, A,
// 5, A
rle_status_t rle_encode(const uint8_t *in, size_t in_len, uint8_t *out,
                        size_t out_cap, size_t *out_len) {
  // TODO: walk the input, measure each run (cap it at 255), emit
  // [count][value].
  // TODO: fail with RLE_ERR_BUFFER_TOO_SMALL before writing past out_cap.
  printf("Encoding started......\n");
  if (in_len == 0) {
    *out_len = 0;
    return RLE_OK;
  }
  size_t count = 1;
  size_t index = 0;
  for (size_t i = 0; i < in_len; i++) {
    if (i + 1 >= in_len ||
        in[i] != in[i + 1]) { /// If you reached the end of in_data or the next
                              /// in_data is not equal to prevous in_data
      if (index + 1 >= out_cap) {
        return RLE_ERR_BUFFER_TOO_SMALL;
      }
      if (count > UINT8_MAX) {
        size_t rep_in = (count - (count % UINT8_MAX)) / UINT8_MAX;
        for (size_t k = 0; k < rep_in; k++) {
          if (index + 1 >= out_cap) {
            return RLE_ERR_BUFFER_TOO_SMALL;
          }
          out[index + 1] = (uint8_t)in[i];
          out[index] = UINT8_MAX;
          index += 2;
        }
        if (count % UINT8_MAX != 0) {
          if (index + 1 >= out_cap) {
            return RLE_ERR_BUFFER_TOO_SMALL;
          }
          out[index + 1] = (uint8_t)in[i];
          out[index] = count % UINT8_MAX;
          index += 2;
        }
        count = 0;
      } else {
        out[index + 1] = (uint8_t)in[i];
        out[index] = count;
        index += 2;
        count = 0;
      }
    }
    count++;
  }
  *out_len = index;
  return RLE_OK;
}

//  encoded data in[0..in_len) into out. On RLE_OK, *out_len is the bytes
// written.
rle_status_t rle_decode(const uint8_t *in, size_t in_len, uint8_t *out,
                        size_t out_cap, size_t *out_len) {
  // TODO: read pairs, repeat each value `count` times.
  // TODO: reject bad input (odd length, count of 0) and never overflow out.
  size_t index = 0;
  size_t count = 0;
  if (in_len % 2 != 0) {
    return RLE_ERR_MALFORMED;
  }
  if (in_len == 0) {
    *out_len = 0;
    return RLE_OK;
  }
  for (size_t i = 0; i < in_len; i += 2) {
    if (in[i] == 0) {
      return RLE_ERR_MALFORMED;
    }
    for (uint8_t j = 0; j < in[i]; j++) {
      if (count >= out_cap) {
        return RLE_ERR_BUFFER_TOO_SMALL;
      }
      out[index + j] = (char)in[i + 1];
      count++;
    }
    index = count;
  }
  *out_len = count;
  return RLE_OK;
}

static void dump(const char *label, const uint8_t *buf, size_t len) {
  printf("%s (%zu bytes):", label, len);
  for (size_t i = 0; i < len; i++)
    printf(" %u", (unsigned)buf[i]);
  printf("\n");
}

int main(void) {
  const uint8_t input[] = "AAAABBBCCD";
  printf("Data to be encoded: %s\n", input);
  size_t input_len = sizeof input - 1; // skip the string's '\0'
  uint8_t encoded[RLE_MAX_ENCODED_SIZE(sizeof input)];
  size_t encoded_len = 0;
  rle_status_t st =
      rle_encode(input, input_len, encoded, sizeof encoded, &encoded_len);
  print_encoding(encoded, &encoded_len, 1);
  printf("encode status: %d\n", (int)st);
  dump("encoded", encoded, encoded_len);

  uint8_t decoded[sizeof input];
  size_t decoded_len = 0;
  st = rle_decode(encoded, encoded_len, decoded, sizeof decoded, &decoded_len);
  print_encoding(decoded, &decoded_len, 0);
  printf("decode status: %d\n", (int)st);
  printf("round trip ok : %s\n",
         decoded_len == input_len && memcmp(decoded, input, input_len) == 0
             ? "yes"
             : "NO");
  return 0;
}
