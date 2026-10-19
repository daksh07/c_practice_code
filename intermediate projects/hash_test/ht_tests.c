// Test harness for the hash table exercise.
//
// Usage:  ./run_tests.sh your_file.c
// It includes your file directly (your main() is renamed out of the way),
// so you don't need to change your code to test it.
//
// Each test runs in its own child process, so a crash in one test
// is reported as CRASH and the remaining tests still run.

#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define main user_main_unused
#include HT_FILE
#undef main

static int checks_failed; // failures inside the current test (child process)

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("      FAIL line %d: %s\n", __LINE__, msg);                       \
      checks_failed++;                                                         \
    }                                                                          \
  } while (0)

// ---------- helpers ----------
static void new_table(hash_table_t *ht, size_t cap) { ht_init(ht, cap); }

// ---------- tests ----------
static void t_init(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  CHECK(ht.buckets != NULL, "buckets allocated");
  CHECK(ht.capacity == 16, "capacity set");
  CHECK(ht.count == 0, "count starts at 0");
  int all_null = 1;
  for (size_t i = 0; i < ht.capacity; i++)
    if (ht.buckets[i] != NULL) all_null = 0;
  CHECK(all_null, "every bucket starts NULL");
  ht_destroy(&ht);
}

static void t_insert_get(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  CHECK(ht_insert(&ht, "name", "Ada") == true, "insert returns true");
  CHECK(ht_insert(&ht, "city", "Pune") == true, "insert returns true");
  CHECK(ht.count == 2, "count == 2");
  CHECK(ht_get(&ht, "name") && strcmp(ht_get(&ht, "name"), "Ada") == 0,
        "get name");
  CHECK(ht_get(&ht, "city") && strcmp(ht_get(&ht, "city"), "Pune") == 0,
        "get city");
  ht_destroy(&ht);
}

static void t_get_missing(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  CHECK(ht_get(&ht, "nope") == NULL, "missing key on empty table is NULL");
  ht_insert(&ht, "a", "1");
  CHECK(ht_get(&ht, "b") == NULL, "missing key is NULL");
  CHECK(ht_get(&ht, "") == NULL, "empty-string key not present is NULL");
  ht_destroy(&ht);
}

static void t_update(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  ht_insert(&ht, "name", "Ada");
  ht_insert(&ht, "name", "Grace");
  CHECK(ht.count == 1, "updating a key must not change count");
  CHECK(ht_get(&ht, "name") && strcmp(ht_get(&ht, "name"), "Grace") == 0,
        "value replaced");
  ht_destroy(&ht);
}

static void t_value_by_reference(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  int x = 42;
  ht_insert(&ht, "x", &x);
  CHECK(ht_get(&ht, "x") == &x, "get returns the same pointer that was stored");
  ht_destroy(&ht);
}

static void t_key_is_copied(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  char buf[16];
  strcpy(buf, "temp");
  ht_insert(&ht, buf, "v");
  strcpy(buf, "XXXX"); // caller reuses its buffer
  CHECK(ht_get(&ht, "temp") != NULL, "key still findable after caller's buffer changed");
  CHECK(ht_get(&ht, "XXXX") == NULL, "table must not see the caller's new text");
  ht_destroy(&ht);
}

static void t_long_keys(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  const char *k = "a_really_long_key_name_that_is_well_over_eight_chars";
  ht_insert(&ht, k, "L");
  CHECK(ht_get(&ht, k) && strcmp(ht_get(&ht, k), "L") == 0, "long key found");
  ht_destroy(&ht); // ASan catches an undersized key allocation here or earlier
}

static void t_similar_keys(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  ht_insert(&ht, "key", "A");
  ht_insert(&ht, "key1", "B");
  ht_insert(&ht, "ke", "C");
  CHECK(strcmp(ht_get(&ht, "key"), "A") == 0, "key");
  CHECK(strcmp(ht_get(&ht, "key1"), "B") == 0, "key1 (prefix must not match)");
  CHECK(strcmp(ht_get(&ht, "ke"), "C") == 0, "ke");
  ht_destroy(&ht);
}

static void t_collisions_capacity1(void) {
  hash_table_t ht;
  new_table(&ht, 1); // every key lands in the same chain
  const char *keys[] = {"one", "two", "three", "four", "five"};
  const char *vals[] = {"1", "2", "3", "4", "5"};
  for (int i = 0; i < 5; i++) ht_insert(&ht, keys[i], (void *)vals[i]);
  CHECK(ht.count == 5, "count == 5");
  for (int i = 0; i < 5; i++) {
    char *v = ht_get(&ht, keys[i]);
    CHECK(v && strcmp(v, vals[i]) == 0, "each key reachable through the chain");
  }
  ht_insert(&ht, "three", "33"); // update in the middle of a chain
  CHECK(ht.count == 5, "update inside chain keeps count");
  CHECK(strcmp(ht_get(&ht, "three"), "33") == 0, "update inside chain works");
  ht_destroy(&ht);
}

static void t_remove_basic(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  ht_insert(&ht, "a", "1");
  CHECK(ht_remove(&ht, "a") == true, "remove existing returns true");
  CHECK(ht.count == 0, "count back to 0");
  CHECK(ht_get(&ht, "a") == NULL, "removed key is gone");
  CHECK(ht_remove(&ht, "a") == false, "removing again returns false");
  CHECK(ht_remove(&ht, "never") == false, "removing unknown key returns false");
  CHECK(ht.count == 0, "failed remove must not change count");
  ht_destroy(&ht);
}

static void t_remove_positions(void) {
  // capacity 1 -> one chain. Inserts push to the front, so the order is
  // five, four, three, two, one (head ... tail).
  hash_table_t ht;
  new_table(&ht, 1);
  const char *keys[] = {"one", "two", "three", "four", "five"};
  for (int i = 0; i < 5; i++) ht_insert(&ht, keys[i], "v");
  CHECK(ht_remove(&ht, "five") == true, "remove head");
  CHECK(ht_remove(&ht, "one") == true, "remove tail");
  CHECK(ht_remove(&ht, "three") == true, "remove middle");
  CHECK(ht.count == 2, "2 left");
  CHECK(ht_get(&ht, "two") != NULL, "two survives");
  CHECK(ht_get(&ht, "four") != NULL, "four survives");
  CHECK(ht_get(&ht, "five") == NULL, "five gone");
  CHECK(ht_get(&ht, "one") == NULL, "one gone");
  CHECK(ht_get(&ht, "three") == NULL, "three gone");
  ht_destroy(&ht);
}

static void t_reinsert_after_remove(void) {
  hash_table_t ht;
  new_table(&ht, 1);
  ht_insert(&ht, "a", "1");
  ht_insert(&ht, "b", "2");
  ht_remove(&ht, "a");
  ht_remove(&ht, "b");
  CHECK(ht.count == 0, "emptied");
  CHECK(ht_insert(&ht, "a", "9") == true, "insert after emptying");
  CHECK(ht.count == 1, "count 1");
  CHECK(strcmp(ht_get(&ht, "a"), "9") == 0, "value correct");
  ht_destroy(&ht);
}

static void t_many_keys(void) {
  hash_table_t ht;
  new_table(&ht, 16); // 1000 keys in 16 buckets: long chains
  char keys[1000][16];
  static char vals[1000][16];
  for (int i = 0; i < 1000; i++) {
    snprintf(keys[i], sizeof keys[i], "key_%d", i);
    snprintf(vals[i], sizeof vals[i], "val_%d", i);
    ht_insert(&ht, keys[i], vals[i]);
  }
  CHECK(ht.count == 1000, "count == 1000");
  int ok = 1;
  for (int i = 0; i < 1000; i++) {
    char *v = ht_get(&ht, keys[i]);
    if (!v || strcmp(v, vals[i]) != 0) ok = 0;
  }
  CHECK(ok, "all 1000 keys retrievable with the right value");
  for (int i = 0; i < 1000; i += 2) ht_remove(&ht, keys[i]);
  CHECK(ht.count == 500, "removed half");
  ok = 1;
  for (int i = 0; i < 1000; i++) {
    int present = ht_get(&ht, keys[i]) != NULL;
    if (present != (i % 2 == 1)) ok = 0;
  }
  CHECK(ok, "exactly the odd keys remain");
  ht_destroy(&ht);
}

static void t_big_capacity(void) {
  // A capacity above 255 catches uint8_t loop counters in destroy.
  hash_table_t ht;
  new_table(&ht, 300);
  for (int i = 0; i < 50; i++) {
    char k[16];
    snprintf(k, sizeof k, "k%d", i);
    ht_insert(&ht, k, "v");
  }
  CHECK(ht.count == 50, "count == 50");
  ht_destroy(&ht); // ASan: leaked nodes or use-after-free show up here
}

static void t_destroy_state(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  ht_insert(&ht, "a", "1");
  ht_insert(&ht, "b", "2");
  ht_destroy(&ht);
  CHECK(ht.buckets == NULL, "buckets reset to NULL after destroy");
  CHECK(ht.capacity == 0, "capacity reset to 0");
  CHECK(ht.count == 0, "count reset to 0");
}

static void t_destroy_empty(void) {
  hash_table_t ht;
  new_table(&ht, 16);
  ht_destroy(&ht);
  CHECK(ht.capacity == 0, "empty table destroyed cleanly");
}

static void t_print_no_crash(void) {
  hash_table_t ht;
  new_table(&ht, 300);
  ht_insert(&ht, "a", "A");
  ht_insert(&ht, "b", "B");
  fflush(stdout);
  int saved = dup(1); // hide print output so it doesn't clutter results
  freopen("/dev/null", "w", stdout);
  print_hash_table(&ht); // must terminate (uint8_t counters loop forever here)
  ht_destroy(&ht);
  print_hash_table(&ht); // destroyed table: must not crash
  fflush(stdout);
  dup2(saved, 1);
  CHECK(1, "print_hash_table terminated");
}

// ---------- runner ----------
typedef struct {
  const char *name;
  void (*fn)(void);
} test_t;

static const test_t tests[] = {
    {"init", t_init},
    {"insert + get", t_insert_get},
    {"get missing key", t_get_missing},
    {"update existing key", t_update},
    {"value stored by reference", t_value_by_reference},
    {"key is copied", t_key_is_copied},
    {"long keys", t_long_keys},
    {"similar keys / prefixes", t_similar_keys},
    {"collisions (capacity 1)", t_collisions_capacity1},
    {"remove basics", t_remove_basic},
    {"remove head / middle / tail", t_remove_positions},
    {"reinsert after remove", t_reinsert_after_remove},
    {"1000 keys, remove half", t_many_keys},
    {"capacity 300 (uint8_t trap)", t_big_capacity},
    {"destroy resets the table", t_destroy_state},
    {"destroy empty table", t_destroy_empty},
    {"print_hash_table terminates", t_print_no_crash},
};

int main(void) {
  int n = (int)(sizeof tests / sizeof tests[0]);
  int passed = 0;
  for (int i = 0; i < n; i++) {
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
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
    } else if (WIFSIGNALED(status)) {
      printf("  CRASH  %s (signal %d)\n", tests[i].name, WTERMSIG(status));
    } else {
      printf("  ERROR  %s (exit %d: sanitizer error or leak, scroll up)\n",
             tests[i].name, WEXITSTATUS(status));
    }
  }
  printf("\n%d/%d tests passed\n", passed, n);
  return passed == n ? 0 : 1;
}
