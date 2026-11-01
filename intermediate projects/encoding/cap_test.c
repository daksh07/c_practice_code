// Small test for the 255 cap in rle_encode.
//
// Build and run (your file name goes in the quotes):
//   cc -fsanitize=address,undefined -DRLE_FILE='"your_rle.c"' cap_test.c -o cap_test && ./cap_test
//
// For each run length it encodes that many 'x' bytes, prints the pairs it got,
// and compares them with the pairs the format requires.

#include <stdio.h>
#include <stdlib.h>

#define main user_main_unused
#include RLE_FILE
#undef main

// Expected pairs for `n` identical bytes: as many 255s as fit, then the rest.
static size_t expected_pairs(size_t n, uint8_t counts[]) {
  size_t k = 0;
  while (n > 255) {
    counts[k++] = 255;
    n -= 255;
  }
  if (n > 0) counts[k++] = (uint8_t)n;
  return k;
}

static int check(size_t n) {
  uint8_t *in = malloc(n);
  for (size_t i = 0; i < n; i++) in[i] = 'x';
  uint8_t out[64];
  size_t out_len = 777; // junk on purpose: the function must set it itself
  rle_status_t st = rle_encode(in, n, out, sizeof out, &out_len);
  free(in);

  uint8_t want[8];
  size_t want_pairs = expected_pairs(n, want);

  int ok = st == RLE_OK && out_len == want_pairs * 2;
  for (size_t i = 0; ok && i < want_pairs; i++)
    ok = out[2 * i] == want[i] && out[2 * i + 1] == 'x';

  printf("%s  %3zu x 'x'  expected:", ok ? "PASS" : "FAIL", n);
  for (size_t i = 0; i < want_pairs; i++) printf(" [%u]", (unsigned)want[i]);
  printf("   got:");
  if (st != RLE_OK) {
    printf(" status %d", (int)st);
  } else {
    for (size_t i = 0; i + 1 < out_len; i += 2) printf(" [%u]", (unsigned)out[i]);
    if (out_len % 2) printf(" (odd length %zu)", out_len);
  }
  printf("\n");
  return ok;
}

int main(void) {
  const size_t lengths[] = {1, 254, 255, 256, 257, 300, 509, 510, 511, 765};
  int passed = 0, total = (int)(sizeof lengths / sizeof lengths[0]);
  for (int i = 0; i < total; i++) passed += check(lengths[i]);
  printf("\n%d/%d cases passed\n", passed, total);
  return passed == total ? 0 : 1;
}
