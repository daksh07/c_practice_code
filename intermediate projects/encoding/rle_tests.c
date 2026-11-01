// Test harness for the run-length encoding exercise.
// Usage:  sh run_tests.sh your_rle.c
// Your main() is ignored. Each test runs in its own process: a crash shows
// as CRASH, an infinite loop as TIMEOUT, and the other tests still run.

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define main user_main_unused
#include RLE_FILE
#undef main

static int checks_failed;

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("      FAIL line %d: %s\n", __LINE__, msg);                       \
      checks_failed++;                                                         \
    }                                                                          \
  } while (0)

#define CANARY 0xA5
#define GUARD 16

// Encode into a buffer of exactly `cap` bytes with guard bytes after it.
// Returns the status; `got` receives the output, `*len` its length.
// Sets *overrun when anything past `cap` was touched.
static rle_status_t enc(const uint8_t *in, size_t n, size_t cap, uint8_t *got,
                        size_t *len, int *overrun) {
  uint8_t *buf = malloc(cap + GUARD);
  memset(buf, CANARY, cap + GUARD);
  *len = 12345;
  rle_status_t st = rle_encode(in, n, buf, cap, len);
  *overrun = 0;
  for (size_t i = cap; i < cap + GUARD; i++)
    if (buf[i] != CANARY) *overrun = 1;
  if (st == RLE_OK && got) memcpy(got, buf, *len <= cap ? *len : cap);
  free(buf);
  return st;
}

static rle_status_t dec(const uint8_t *in, size_t n, size_t cap, uint8_t *got,
                        size_t *len, int *overrun) {
  uint8_t *buf = malloc(cap + GUARD);
  memset(buf, CANARY, cap + GUARD);
  *len = 12345;
  rle_status_t st = rle_decode(in, n, buf, cap, len);
  *overrun = 0;
  for (size_t i = cap; i < cap + GUARD; i++)
    if (buf[i] != CANARY) *overrun = 1;
  if (st == RLE_OK && got) memcpy(got, buf, *len <= cap ? *len : cap);
  free(buf);
  return st;
}

// Fill buf with n copies of v.
static void fill(uint8_t *buf, size_t n, uint8_t v) { memset(buf, v, n); }

// Round trip: encode then decode, expect the original back.
static int roundtrip(const uint8_t *in, size_t n) {
  size_t cap = RLE_MAX_ENCODED_SIZE(n) + 2;
  uint8_t *e = malloc(cap), *d = malloc(n + 1);
  size_t elen = 0, dlen = 0;
  int ok = rle_encode(in, n, e, cap, &elen) == RLE_OK &&
           rle_decode(e, elen, d, n + 1, &dlen) == RLE_OK && dlen == n &&
           (n == 0 || memcmp(d, in, n) == 0);
  free(e);
  free(d);
  return ok;
}

// ---------- encode tests ----------
static void t_enc_example(void) {
  const uint8_t in[] = "AAAABBBCCD";
  const uint8_t want[] = {4, 'A', 3, 'B', 2, 'C', 1, 'D'};
  uint8_t got[32];
  size_t len;
  int over;
  CHECK(enc(in, 10, 32, got, &len, &over) == RLE_OK, "status OK");
  CHECK(len == sizeof want, "length is 8");
  CHECK(len == sizeof want && memcmp(got, want, sizeof want) == 0,
        "bytes are 4A 3B 2C 1D");
}

static void t_enc_empty(void) {
  uint8_t got[4];
  size_t len;
  int over;
  const uint8_t in[1] = {0};
  CHECK(enc(in, 0, 4, got, &len, &over) == RLE_OK, "empty input is OK");
  CHECK(len == 0, "empty input gives 0 bytes");
  CHECK(!over, "nothing written");
}

static void t_enc_single(void) {
  const uint8_t in[] = {0x7E};
  uint8_t got[4];
  size_t len;
  int over;
  CHECK(enc(in, 1, 4, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 2 && got[0] == 1 && got[1] == 0x7E, "one byte -> [1][byte]");
}

static void t_enc_all_same(void) {
  uint8_t in[50];
  fill(in, 50, 9);
  uint8_t got[8];
  size_t len;
  int over;
  CHECK(enc(in, 50, 8, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 2 && got[0] == 50 && got[1] == 9, "50 x 9 -> [50][9]");
}

static void t_enc_run_255(void) {
  uint8_t in[255];
  fill(in, 255, 'x');
  uint8_t got[8];
  size_t len;
  int over;
  CHECK(enc(in, 255, 8, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 2 && got[0] == 255 && got[1] == 'x', "255 fits one pair");
}

static void t_enc_run_256(void) {
  uint8_t in[256];
  fill(in, 256, 'x');
  uint8_t got[8];
  size_t len;
  int over;
  CHECK(enc(in, 256, 8, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 4, "256 needs two pairs");
  CHECK(len == 4 && got[0] == 255 && got[1] == 'x' && got[2] == 1 &&
            got[3] == 'x',
        "[255]x [1]x");
}

static void t_enc_run_300(void) {
  uint8_t in[300];
  fill(in, 300, 7);
  uint8_t got[8];
  size_t len;
  int over;
  CHECK(enc(in, 300, 8, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 4 && got[0] == 255 && got[2] == 45, "300 -> 255 + 45");
}

static void t_enc_run_510(void) {
  uint8_t in[510];
  fill(in, 510, 3);
  uint8_t got[8];
  size_t len;
  int over;
  CHECK(enc(in, 510, 8, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 4 && got[0] == 255 && got[2] == 255, "510 -> 255 + 255");
}

static void t_enc_no_repeats(void) {
  const uint8_t in[] = {1, 2, 3, 4, 5};
  uint8_t got[16];
  size_t len;
  int over;
  CHECK(enc(in, 5, 10, got, &len, &over) == RLE_OK, "worst case fits 2x");
  CHECK(len == 10, "no repeats doubles the size");
  int ok = len == 10;
  for (int i = 0; i < 5 && ok; i++) ok = got[2 * i] == 1 && got[2 * i + 1] == i + 1;
  CHECK(ok, "each byte is its own [1][v]");
}

static void t_enc_alternating(void) {
  const uint8_t in[] = {'a', 'b', 'a', 'b', 'a', 'b'};
  uint8_t got[16];
  size_t len;
  int over;
  CHECK(enc(in, 6, 12, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 12, "alternating bytes never merge");
}

static void t_enc_binary_values(void) {
  const uint8_t in[] = {0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00};
  const uint8_t want[] = {3, 0x00, 2, 0xFF, 1, 0x00};
  uint8_t got[16];
  size_t len;
  int over;
  CHECK(enc(in, 6, 16, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 6 && memcmp(got, want, 6) == 0,
        "0x00 and 0xFF are ordinary values");
}

static void t_enc_last_run(void) {
  // The final run is the classic one to forget.
  const uint8_t in[] = {1, 1, 2, 2, 2};
  uint8_t got[16];
  size_t len;
  int over;
  CHECK(enc(in, 5, 16, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 4 && got[0] == 2 && got[1] == 1 && got[2] == 3 && got[3] == 2,
        "the last run is written too");
}

static void t_enc_exact_fit(void) {
  const uint8_t in[] = {1, 1, 2};
  uint8_t got[8];
  size_t len;
  int over;
  CHECK(enc(in, 3, 4, got, &len, &over) == RLE_OK, "exactly enough space is OK");
  CHECK(len == 4, "4 bytes written");
  CHECK(!over, "nothing written past out_cap");
}

static void t_enc_too_small(void) {
  const uint8_t in[] = {1, 1, 2};
  size_t len;
  int over;
  CHECK(enc(in, 3, 3, NULL, &len, &over) == RLE_ERR_BUFFER_TOO_SMALL,
        "one byte short -> BUFFER_TOO_SMALL");
  CHECK(!over, "nothing written past out_cap");
  CHECK(enc(in, 3, 0, NULL, &len, &over) == RLE_ERR_BUFFER_TOO_SMALL,
        "zero capacity -> BUFFER_TOO_SMALL");
  CHECK(!over, "nothing written with zero capacity");
  CHECK(enc(in, 3, 1, NULL, &len, &over) == RLE_ERR_BUFFER_TOO_SMALL,
        "1 byte of room is not enough for a pair");
  CHECK(!over, "no half pair written past out_cap");
}

static void t_enc_odd_capacity(void) {
  // Room for 1.5 pairs: the second pair must not be started.
  const uint8_t in[] = {1, 2};
  size_t len;
  int over;
  CHECK(enc(in, 2, 3, NULL, &len, &over) == RLE_ERR_BUFFER_TOO_SMALL,
        "odd capacity too small for two pairs");
  CHECK(!over, "no write past out_cap");
}

// ---------- decode tests ----------
static void t_dec_example(void) {
  const uint8_t in[] = {4, 'A', 3, 'B', 2, 'C', 1, 'D'};
  uint8_t got[32];
  size_t len;
  int over;
  CHECK(dec(in, 8, 32, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 10 && memcmp(got, "AAAABBBCCD", 10) == 0, "AAAABBBCCD");
}

static void t_dec_empty(void) {
  const uint8_t in[1] = {0};
  uint8_t got[4];
  size_t len;
  int over;
  CHECK(dec(in, 0, 4, got, &len, &over) == RLE_OK, "empty is OK");
  CHECK(len == 0, "empty gives 0 bytes");
}

static void t_dec_big_run(void) {
  const uint8_t in[] = {255, 'z', 45, 'z'};
  uint8_t got[400];
  size_t len;
  int over;
  CHECK(dec(in, 4, 400, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 300, "255 + 45 = 300");
  int ok = len == 300;
  for (size_t i = 0; i < len && ok; i++) ok = got[i] == 'z';
  CHECK(ok, "all z");
}

static void t_dec_binary_values(void) {
  const uint8_t in[] = {3, 0x00, 2, 0xFF};
  const uint8_t want[] = {0, 0, 0, 0xFF, 0xFF};
  uint8_t got[16];
  size_t len;
  int over;
  CHECK(dec(in, 4, 16, got, &len, &over) == RLE_OK, "OK");
  CHECK(len == 5 && memcmp(got, want, 5) == 0, "zeros and 0xFF come through");
}

static void t_dec_odd_length(void) {
  const uint8_t in[] = {2, 'a', 3};
  size_t len;
  int over;
  CHECK(dec(in, 3, 64, NULL, &len, &over) == RLE_ERR_MALFORMED,
        "odd input length is malformed");
  CHECK(!over, "no overrun");
}

static void t_dec_zero_count(void) {
  const uint8_t in[] = {0, 'a'};
  size_t len;
  int over;
  CHECK(dec(in, 2, 64, NULL, &len, &over) == RLE_ERR_MALFORMED,
        "count of 0 is malformed");
  const uint8_t in2[] = {2, 'a', 0, 'b'};
  CHECK(dec(in2, 4, 64, NULL, &len, &over) == RLE_ERR_MALFORMED,
        "count of 0 later on is malformed");
}

static void t_dec_exact_fit(void) {
  const uint8_t in[] = {3, 'a'};
  uint8_t got[8];
  size_t len;
  int over;
  CHECK(dec(in, 2, 3, got, &len, &over) == RLE_OK, "exact fit is OK");
  CHECK(len == 3 && !over, "3 bytes, nothing past out_cap");
}

static void t_dec_too_small(void) {
  const uint8_t in[] = {3, 'a', 2, 'b'};
  size_t len;
  int over;
  CHECK(dec(in, 4, 4, NULL, &len, &over) == RLE_ERR_BUFFER_TOO_SMALL,
        "5 bytes into 4 -> BUFFER_TOO_SMALL");
  CHECK(!over, "nothing written past out_cap (second run)");
  CHECK(dec(in, 4, 2, NULL, &len, &over) == RLE_ERR_BUFFER_TOO_SMALL,
        "first run does not fit");
  CHECK(!over, "nothing written past out_cap (first run)");
  CHECK(dec(in, 4, 0, NULL, &len, &over) == RLE_ERR_BUFFER_TOO_SMALL,
        "zero capacity");
  CHECK(!over, "nothing written with zero capacity");
}

static void t_dec_huge_run_small_buffer(void) {
  const uint8_t in[] = {255, 'a'};
  size_t len;
  int over;
  CHECK(dec(in, 2, 10, NULL, &len, &over) == RLE_ERR_BUFFER_TOO_SMALL,
        "a 255 run into 10 bytes fails");
  CHECK(!over, "stopped before overflowing");
}

// ---------- round trips ----------
static void t_rt_small(void) {
  const uint8_t a[] = {1};
  const uint8_t b[] = {1, 1};
  const uint8_t c[] = {1, 2};
  const uint8_t d[] = {0, 0, 0, 0};
  const uint8_t e[] = {255, 255, 0, 255};
  CHECK(roundtrip(a, 1), "1");
  CHECK(roundtrip(b, 2), "1 1");
  CHECK(roundtrip(c, 2), "1 2");
  CHECK(roundtrip(d, 4), "0 0 0 0");
  CHECK(roundtrip(e, 4), "255 255 0 255");
  CHECK(roundtrip(a, 0), "empty");
}

static void t_rt_run_lengths(void) {
  // Every run length around the 255 boundary.
  int ok = 1;
  for (size_t n = 1; n <= 520; n++) {
    uint8_t *buf = malloc(n);
    memset(buf, 0x41, n);
    if (!roundtrip(buf, n)) ok = 0;
    free(buf);
  }
  CHECK(ok, "runs of every length 1..520 round trip");
}

static void t_rt_random_runs(void) {
  unsigned seed = 12345;
  int ok = 1;
  for (int round = 0; round < 200; round++) {
    uint8_t buf[3000];
    size_t n = 0;
    while (n < 2500) {
      seed = seed * 1103515245u + 12345u;
      size_t run = 1 + (seed >> 16) % 600;
      seed = seed * 1103515245u + 12345u;
      uint8_t v = (uint8_t)(seed >> 16);
      for (size_t k = 0; k < run && n < sizeof buf; k++) buf[n++] = v;
    }
    if (!roundtrip(buf, n)) ok = 0;
  }
  CHECK(ok, "200 random run-heavy inputs round trip");
}

static void t_rt_noise(void) {
  unsigned seed = 99;
  int ok = 1;
  for (int round = 0; round < 100; round++) {
    uint8_t buf[500];
    for (size_t i = 0; i < sizeof buf; i++) {
      seed = seed * 1103515245u + 12345u;
      buf[i] = (uint8_t)((seed >> 16) & 3); // few distinct values -> some runs
    }
    if (!roundtrip(buf, sizeof buf)) ok = 0;
  }
  CHECK(ok, "100 noisy inputs round trip");
}

// ---------- runner ----------
typedef struct {
  const char *name;
  void (*fn)(void);
} test_t;

static const test_t tests[] = {
    {"encode: the AAAABBBCCD example", t_enc_example},
    {"encode: empty input", t_enc_empty},
    {"encode: single byte", t_enc_single},
    {"encode: all the same byte", t_enc_all_same},
    {"encode: run of exactly 255", t_enc_run_255},
    {"encode: run of 256 splits", t_enc_run_256},
    {"encode: run of 300 splits", t_enc_run_300},
    {"encode: run of 510 splits twice", t_enc_run_510},
    {"encode: no repeats (2x worst case)", t_enc_no_repeats},
    {"encode: alternating bytes", t_enc_alternating},
    {"encode: 0x00 and 0xFF values", t_enc_binary_values},
    {"encode: the last run is written", t_enc_last_run},
    {"encode: exact-fit buffer", t_enc_exact_fit},
    {"encode: buffer too small", t_enc_too_small},
    {"encode: odd capacity", t_enc_odd_capacity},
    {"decode: the example", t_dec_example},
    {"decode: empty input", t_dec_empty},
    {"decode: run of 255 + 45", t_dec_big_run},
    {"decode: 0x00 and 0xFF values", t_dec_binary_values},
    {"decode: odd length is malformed", t_dec_odd_length},
    {"decode: count 0 is malformed", t_dec_zero_count},
    {"decode: exact-fit buffer", t_dec_exact_fit},
    {"decode: buffer too small", t_dec_too_small},
    {"decode: big run into tiny buffer", t_dec_huge_run_small_buffer},
    {"round trip: small cases", t_rt_small},
    {"round trip: run lengths 1..520", t_rt_run_lengths},
    {"round trip: random runs", t_rt_random_runs},
    {"round trip: noisy data", t_rt_noise},
};

int main(void) {
  int n = (int)(sizeof tests / sizeof tests[0]);
  int passed = 0;
  for (int i = 0; i < n; i++) {
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
      alarm(5);
      checks_failed = 0;
      tests[i].fn();
      fflush(stdout);
      _exit(checks_failed ? 1 : 0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      printf("  PASS   %s\n", tests[i].name);
      passed++;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
      printf("  FAIL   %s\n", tests[i].name);
    } else if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
      printf("  TIMEOUT %s (probably an infinite loop)\n", tests[i].name);
    } else if (WIFSIGNALED(status)) {
      printf("  CRASH  %s (signal %d)\n", tests[i].name, WTERMSIG(status));
    } else {
      printf("  ERROR  %s (exit %d: sanitizer report above)\n", tests[i].name,
             WEXITSTATUS(status));
    }
  }
  printf("\n%d/%d tests passed\n", passed, n);
  return passed == n ? 0 : 1;
}
