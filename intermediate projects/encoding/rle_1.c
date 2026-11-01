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

// Compress in[0..in_len) into out. On RLE_OK, *out_len is the bytes written.
rle_status_t rle_encode(const uint8_t *in, size_t in_len, uint8_t *out,
                        size_t out_cap, size_t *out_len) {
  // TODO: walk the input, measure each run (cap it at 255), emit [count][value].
  // TODO: fail with RLE_ERR_BUFFER_TOO_SMALL before writing past out_cap.
  return RLE_ERR_MALFORMED;
}

// Expand encoded data in[0..in_len) into out. On RLE_OK, *out_len is the bytes
// written.
rle_status_t rle_decode(const uint8_t *in, size_t in_len, uint8_t *out,
                        size_t out_cap, size_t *out_len) {
  // TODO: read pairs, repeat each value `count` times.
  // TODO: reject bad input (odd length, count of 0) and never overflow out.
  return RLE_ERR_BUFFER_TOO_SMALL;
}

static void dump(const char *label, const uint8_t *buf, size_t len) {
  printf("%s (%zu bytes):", label, len);
  for (size_t i = 0; i < len; i++) printf(" %u", (unsigned)buf[i]);
  printf("\n");
}

int main(void) {
  const uint8_t input[] = "AAAABBBCCD";
  size_t input_len = sizeof input - 1; // skip the string's '\0'

  uint8_t encoded[RLE_MAX_ENCODED_SIZE(sizeof input)];
  size_t encoded_len = 0;
  rle_status_t st = rle_encode(input, input_len, encoded, sizeof encoded, &encoded_len);
  printf("encode status: %d\n", (int)st);
  dump("encoded", encoded, encoded_len);

  uint8_t decoded[sizeof input];
  size_t decoded_len = 0;
  st = rle_decode(encoded, encoded_len, decoded, sizeof decoded, &decoded_len);
  printf("decode status: %d\n", (int)st);
  printf("round trip ok: %s\n",
         decoded_len == input_len && memcmp(decoded, input, input_len) == 0 ? "yes" : "NO");
  return 0;
}
