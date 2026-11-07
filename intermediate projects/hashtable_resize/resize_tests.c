// Test harness for hash table resize. Do not edit.
// Run: sh run_tests.sh your_file.c
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define main user_main_unused
#include HT_FILE
#undef main

#define CHECK(cond) do { if (!(cond)) { \
  fprintf(stderr, "    line %d: %s\n", __LINE__, #cond); _exit(1); } } while (0)

static char names[2000][12];
static void make_names(void) {
  for (int i = 0; i < 2000; i++) snprintf(names[i], sizeof names[i], "key%d", i);
}
static void fill(hash_table_t *ht, int n) {
  for (int i = 0; i < n; i++) CHECK(ht_insert(ht, names[i], names[i]));
}
static int all_found(hash_table_t *ht, int n) {
  for (int i = 0; i < n; i++) if (ht_get(ht, names[i]) != names[i]) return 0;
  return 1;
}
// every node sits in the bucket its key hashes to, and the node total == count
static int layout_ok(hash_table_t *ht) {
  size_t total = 0;
  for (size_t b = 0; b < ht->capacity; b++)
    for (hash_node_t *n = ht->buckets[b]; n; n = n->next) {
      if (hash_string(n->key) % ht->capacity != b) return 0;
      total++;
      if (total > 100000) return 0;
    }
  return total == ht->count;
}

static void t_manual_grow(void) {
  hash_table_t ht; ht_init(&ht, 4); fill(&ht, 10);
  CHECK(ht_resize(&ht, 64));
  CHECK(ht.capacity == 64); CHECK(ht.count == 10);
  CHECK(all_found(&ht, 10)); CHECK(layout_ok(&ht));
  ht_destroy(&ht);
}
static void t_manual_shrink(void) {
  hash_table_t ht; ht_init(&ht, 64); fill(&ht, 10);
  CHECK(ht_resize(&ht, 3));
  CHECK(ht.capacity == 3); CHECK(ht.count == 10);
  CHECK(all_found(&ht, 10)); CHECK(layout_ok(&ht));
  ht_destroy(&ht);
}
static void t_resize_to_one(void) {
  hash_table_t ht; ht_init(&ht, 8); fill(&ht, 6);
  CHECK(ht_resize(&ht, 1));
  CHECK(ht.capacity == 1); CHECK(all_found(&ht, 6)); CHECK(layout_ok(&ht));
  ht_destroy(&ht);
}
static void t_resize_zero(void) {
  hash_table_t ht; ht_init(&ht, 8); fill(&ht, 5);
  hash_node_t **old = ht.buckets;
  CHECK(!ht_resize(&ht, 0));
  CHECK(ht.buckets == old); CHECK(ht.capacity == 8); CHECK(ht.count == 5);
  CHECK(all_found(&ht, 5));
  ht_destroy(&ht);
}
static void t_resize_empty(void) {
  hash_table_t ht; ht_init(&ht, 4);
  CHECK(ht_resize(&ht, 32));
  CHECK(ht.capacity == 32); CHECK(ht.count == 0);
  fill(&ht, 3); CHECK(all_found(&ht, 3));
  ht_destroy(&ht);
}
static void t_nodes_moved(void) {
  hash_table_t ht; ht_init(&ht, 4); fill(&ht, 3);
  hash_node_t *before[3];
  for (int i = 0; i < 3; i++) {
    hash_node_t *n = ht.buckets[hash_string(names[i]) % 4];
    while (n && strcmp(n->key, names[i])) n = n->next;
    CHECK(n); before[i] = n;
  }
  CHECK(ht_resize(&ht, 64));
  for (int i = 0; i < 3; i++) {
    hash_node_t *n = ht.buckets[hash_string(names[i]) % 64];
    while (n && strcmp(n->key, names[i])) n = n->next;
    CHECK(n == before[i]);
  }
  ht_destroy(&ht);
}
static void t_threshold_16(void) {
  hash_table_t ht; ht_init(&ht, 16);
  fill(&ht, 12); CHECK(ht.capacity == 16);
  fill(&ht, 13); CHECK(ht.capacity == 32);
  CHECK(ht.count == 13); CHECK(all_found(&ht, 13)); CHECK(layout_ok(&ht));
  ht_destroy(&ht);
}
static void t_threshold_32_64(void) {
  hash_table_t ht; ht_init(&ht, 16);
  fill(&ht, 24); CHECK(ht.capacity == 32);
  fill(&ht, 25); CHECK(ht.capacity == 64);
  CHECK(all_found(&ht, 25)); CHECK(layout_ok(&ht));
  ht_destroy(&ht);
}
static void t_small_start(void) {
  hash_table_t ht; ht_init(&ht, 1);
  CHECK(ht_insert(&ht, names[0], names[0]));
  CHECK(ht.capacity == 2); // 1*4 > 1*3
  fill(&ht, 20); CHECK(all_found(&ht, 20)); CHECK(layout_ok(&ht));
  ht_destroy(&ht);
}
static void t_update_no_grow(void) {
  hash_table_t ht; ht_init(&ht, 16); fill(&ht, 12);
  for (int r = 0; r < 5; r++) for (int i = 0; i < 12; i++)
    CHECK(ht_insert(&ht, names[i], names[i + 100]));
  CHECK(ht.capacity == 16); CHECK(ht.count == 12);
  CHECK(ht_get(&ht, names[3]) == names[103]);
  ht_destroy(&ht);
}
static void t_remove_no_shrink(void) {
  hash_table_t ht; ht_init(&ht, 16); fill(&ht, 100);
  size_t cap = ht.capacity;
  for (int i = 0; i < 100; i++) CHECK(ht_remove(&ht, names[i]));
  CHECK(ht.capacity == cap); CHECK(ht.count == 0);
  ht_destroy(&ht);
}
static void t_invariant(void) {
  hash_table_t ht; ht_init(&ht, 4);
  for (int i = 0; i < 1000; i++) {
    CHECK(ht_insert(&ht, names[i], names[i]));
    CHECK(ht.count * 4 <= ht.capacity * 3);
  }
  CHECK(all_found(&ht, 1000)); CHECK(layout_ok(&ht)); CHECK(ht.count == 1000);
  ht_destroy(&ht);
}
static void t_remove_after_growth(void) {
  hash_table_t ht; ht_init(&ht, 4); fill(&ht, 200);
  for (int i = 0; i < 200; i += 2) CHECK(ht_remove(&ht, names[i]));
  CHECK(ht.count == 100);
  for (int i = 0; i < 200; i++)
    CHECK((ht_get(&ht, names[i]) != NULL) == (i % 2 == 1));
  CHECK(layout_ok(&ht));
  ht_destroy(&ht);
}
static void t_insert_after_manual(void) {
  hash_table_t ht; ht_init(&ht, 4); fill(&ht, 3);
  CHECK(ht_resize(&ht, 128));
  for (int i = 3; i < 50; i++) CHECK(ht_insert(&ht, names[i], names[i]));
  CHECK(ht.capacity == 128); CHECK(all_found(&ht, 50)); CHECK(layout_ok(&ht));
  ht_destroy(&ht);
}

typedef struct { const char *name; void (*fn)(void); } test_t;
static const test_t tests[] = {
  {"manual grow keeps entries", t_manual_grow},
  {"manual shrink keeps entries", t_manual_shrink},
  {"resize to capacity 1", t_resize_to_one},
  {"resize to 0 fails, table untouched", t_resize_zero},
  {"resize empty table", t_resize_empty},
  {"nodes are moved, not copied", t_nodes_moved},
  {"16 buckets: 12 keys stay, 13th doubles", t_threshold_16},
  {"32 buckets: 24 stay, 25th doubles", t_threshold_32_64},
  {"starting at capacity 1", t_small_start},
  {"updating a key never grows", t_update_no_grow},
  {"removing never shrinks", t_remove_no_shrink},
  {"1000 inserts keep load <= 3/4", t_invariant},
  {"remove after growth", t_remove_after_growth},
  {"insert after manual resize", t_insert_after_manual},
};

int main(void) {
  make_names();
  int total = (int)(sizeof tests / sizeof tests[0]), passed = 0;
  for (int i = 0; i < total; i++) {
    fflush(stdout);
    pid_t p = fork();
    if (p == 0) { alarm(5); tests[i].fn(); _exit(0); }
    int st; waitpid(p, &st, 0);
    if (WIFEXITED(st) && WEXITSTATUS(st) == 0) { printf("PASS  %s\n", tests[i].name); passed++; }
    else if (WIFSIGNALED(st) && WTERMSIG(st) == SIGALRM) printf("TIMEOUT  %s\n", tests[i].name);
    else if (WIFSIGNALED(st)) printf("CRASH (signal %d)  %s\n", WTERMSIG(st), tests[i].name);
    else printf("FAIL  %s\n", tests[i].name);
  }
  printf("\n%d/%d tests passed\n", passed, total);
  return passed == total ? 0 : 1;
}
