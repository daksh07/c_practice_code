// Test harness for the open-addressing hash table. Do not edit.
// Run: sh run_tests.sh your_file.c
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define main user_main_unused
#include OA_FILE
#undef main

#define CHECK(cond) do { if (!(cond)) { \
  fprintf(stderr, "    line %d: %s\n", __LINE__, #cond); _exit(1); } } while (0)

// Find n keys whose home slot is `home` for a table of `cap` slots.
static void collide(size_t cap, size_t home, int n, char out[][16]) {
  int found = 0;
  for (int i = 0; found < n && i < 1000000; i++) {
    char k[16]; snprintf(k, sizeof k, "k%d", i);
    if (hash_string(k) % cap == home) strcpy(out[found++], k);
  }
}
static int slot_of(oa_table_t *t, const char *key) {
  for (size_t i = 0; i < t->capacity; i++)
    if (t->slots[i].state == SLOT_OCCUPIED && !strcmp(t->slots[i].key, key)) return (int)i;
  return -1;
}
static int count_key(oa_table_t *t, const char *key) {
  int c = 0;
  for (size_t i = 0; i < t->capacity; i++)
    if (t->slots[i].state == SLOT_OCCUPIED && !strcmp(t->slots[i].key, key)) c++;
  return c;
}
static int occupied(oa_table_t *t) {
  int c = 0;
  for (size_t i = 0; i < t->capacity; i++) if (t->slots[i].state == SLOT_OCCUPIED) c++;
  return c;
}
static int vals[512];
static char names[512][12];

static void t_basic(void) {
  oa_table_t t; CHECK(oa_init(&t, 8));
  CHECK(oa_get(&t, "nothing") == NULL);
  CHECK(oa_insert(&t, "a", &vals[1]) == OA_OK);
  CHECK(oa_insert(&t, "b", &vals[2]) == OA_OK);
  CHECK(oa_get(&t, "a") == &vals[1]); CHECK(oa_get(&t, "b") == &vals[2]);
  CHECK(oa_get(&t, "c") == NULL); CHECK(t.count == 2);
  oa_destroy(&t);
}
static void t_update(void) {
  oa_table_t t; CHECK(oa_init(&t, 8));
  CHECK(oa_insert(&t, "a", &vals[1]) == OA_OK);
  CHECK(oa_insert(&t, "a", &vals[2]) == OA_OK);
  CHECK(oa_get(&t, "a") == &vals[2]); CHECK(t.count == 1);
  CHECK(count_key(&t, "a") == 1);
  oa_destroy(&t);
}
static void t_collide(void) {
  oa_table_t t; CHECK(oa_init(&t, 8));
  char k[4][16]; collide(8, 3, 4, k);
  for (int i = 0; i < 4; i++) CHECK(oa_insert(&t, k[i], &vals[i]) == OA_OK);
  for (int i = 0; i < 4; i++) CHECK(oa_get(&t, k[i]) == &vals[i]);
  CHECK(slot_of(&t, k[0]) == 3); CHECK(slot_of(&t, k[1]) == 4);
  CHECK(slot_of(&t, k[2]) == 5); CHECK(slot_of(&t, k[3]) == 6);
  oa_destroy(&t);
}
static void t_wrap(void) {
  oa_table_t t; CHECK(oa_init(&t, 8));
  char k[3][16]; collide(8, 7, 3, k);
  for (int i = 0; i < 3; i++) CHECK(oa_insert(&t, k[i], &vals[i]) == OA_OK);
  CHECK(slot_of(&t, k[0]) == 7); CHECK(slot_of(&t, k[1]) == 0); CHECK(slot_of(&t, k[2]) == 1);
  for (int i = 0; i < 3; i++) CHECK(oa_get(&t, k[i]) == &vals[i]);
  oa_destroy(&t);
}
static void t_key_copied(void) {
  oa_table_t t; CHECK(oa_init(&t, 8));
  char buf[16] = "hello";
  CHECK(oa_insert(&t, buf, &vals[1]) == OA_OK);
  strcpy(buf, "XXXXX");
  CHECK(oa_get(&t, "hello") == &vals[1]); CHECK(oa_get(&t, "XXXXX") == NULL);
  oa_destroy(&t);
}
static void t_fill(void) {
  oa_table_t t; CHECK(oa_init(&t, 16));
  for (int i = 0; i < 16; i++) CHECK(oa_insert(&t, names[i], &vals[i]) == OA_OK);
  CHECK(t.count == 16);
  for (int i = 0; i < 16; i++) CHECK(oa_get(&t, names[i]) == &vals[i]);
  oa_destroy(&t);
}
static void t_full(void) {
  oa_table_t t; CHECK(oa_init(&t, 8));
  for (int i = 0; i < 8; i++) CHECK(oa_insert(&t, names[i], &vals[i]) == OA_OK);
  CHECK(oa_insert(&t, names[8], &vals[8]) == OA_ERR_FULL);
  CHECK(t.count == 8); CHECK(occupied(&t) == 8);
  for (int i = 0; i < 8; i++) CHECK(oa_get(&t, names[i]) == &vals[i]);
  oa_destroy(&t);
}
static void t_update_full(void) {
  oa_table_t t; CHECK(oa_init(&t, 8));
  for (int i = 0; i < 8; i++) CHECK(oa_insert(&t, names[i], &vals[i]) == OA_OK);
  CHECK(oa_insert(&t, names[5], &vals[100]) == OA_OK);
  CHECK(oa_get(&t, names[5]) == &vals[100]); CHECK(t.count == 8);
  oa_destroy(&t);
}
static void t_missing_full(void) {   // no EMPTY slot anywhere: must still stop
  oa_table_t t; CHECK(oa_init(&t, 8));
  for (int i = 0; i < 8; i++) CHECK(oa_insert(&t, names[i], &vals[i]) == OA_OK);
  CHECK(oa_get(&t, "not-there") == NULL);
  CHECK(!oa_remove(&t, "not-there"));
  oa_destroy(&t);
}
static void t_remove(void) {
  oa_table_t t; CHECK(oa_init(&t, 8));
  CHECK(oa_insert(&t, "a", &vals[1]) == OA_OK);
  CHECK(oa_remove(&t, "a")); CHECK(oa_get(&t, "a") == NULL); CHECK(t.count == 0);
  CHECK(!oa_remove(&t, "a")); CHECK(!oa_remove(&t, "zzz")); CHECK(t.count == 0);
  oa_destroy(&t);
}
static void t_remove_chain(void) {   // removing the middle must not hide the keys after it
  oa_table_t t; CHECK(oa_init(&t, 8));
  char k[3][16]; collide(8, 2, 3, k);
  for (int i = 0; i < 3; i++) CHECK(oa_insert(&t, k[i], &vals[i]) == OA_OK);
  CHECK(oa_remove(&t, k[1]));
  CHECK(oa_get(&t, k[2]) == &vals[2]); CHECK(oa_get(&t, k[0]) == &vals[0]);
  CHECK(oa_get(&t, k[1]) == NULL); CHECK(t.count == 2);
  CHECK(oa_remove(&t, k[2]));
  oa_destroy(&t);
}
static void t_reuse(void) {          // a new key reuses the removed slot
  oa_table_t t; CHECK(oa_init(&t, 8));
  char k[4][16]; collide(8, 2, 4, k);
  for (int i = 0; i < 3; i++) CHECK(oa_insert(&t, k[i], &vals[i]) == OA_OK);
  int where = slot_of(&t, k[1]);
  CHECK(oa_remove(&t, k[1]));
  CHECK(oa_insert(&t, k[3], &vals[3]) == OA_OK);
  CHECK(slot_of(&t, k[3]) == where); CHECK(t.count == 3);
  CHECK(oa_get(&t, k[2]) == &vals[2]); CHECK(oa_get(&t, k[3]) == &vals[3]);
  oa_destroy(&t);
}
static void t_no_duplicate(void) {   // key exists past a tombstone: update it, don't add a copy
  oa_table_t t; CHECK(oa_init(&t, 8));
  char k[3][16]; collide(8, 2, 3, k);
  for (int i = 0; i < 3; i++) CHECK(oa_insert(&t, k[i], &vals[i]) == OA_OK);
  CHECK(oa_remove(&t, k[0]));
  CHECK(oa_insert(&t, k[2], &vals[50]) == OA_OK);
  CHECK(count_key(&t, k[2]) == 1); CHECK(t.count == 2); CHECK(occupied(&t) == 2);
  CHECK(oa_get(&t, k[2]) == &vals[50]);
  oa_destroy(&t);
}
static void t_tomb_full(void) {      // full of keys and tombstones, no EMPTY slot
  oa_table_t t; CHECK(oa_init(&t, 8));
  for (int i = 0; i < 8; i++) CHECK(oa_insert(&t, names[i], &vals[i]) == OA_OK);
  CHECK(oa_remove(&t, names[3]));
  CHECK(oa_get(&t, "not-there") == NULL);
  CHECK(oa_insert(&t, names[20], &vals[20]) == OA_OK);
  CHECK(oa_get(&t, names[20]) == &vals[20]); CHECK(t.count == 8);
  CHECK(oa_insert(&t, names[21], &vals[21]) == OA_ERR_FULL);
  oa_destroy(&t);
}
static void t_all_tombstones(void) { // every slot a tombstone
  oa_table_t t; CHECK(oa_init(&t, 8));
  for (int i = 0; i < 8; i++) CHECK(oa_insert(&t, names[i], &vals[i]) == OA_OK);
  for (int i = 0; i < 8; i++) CHECK(oa_remove(&t, names[i]));
  CHECK(t.count == 0); CHECK(oa_get(&t, names[0]) == NULL);
  for (int i = 10; i < 18; i++) CHECK(oa_insert(&t, names[i], &vals[i]) == OA_OK);
  CHECK(t.count == 8);
  for (int i = 10; i < 18; i++) CHECK(oa_get(&t, names[i]) == &vals[i]);
  oa_destroy(&t);
}
static void t_stress(void) {         // compare with a simple model
  enum { CAP = 64, POOL = 50 };
  oa_table_t t; CHECK(oa_init(&t, CAP));
  void *model[POOL] = {0}; int in[POOL] = {0}; int n = 0;
  unsigned seed = 12345;
  for (int step = 0; step < 6000; step++) {
    seed = seed * 1103515245u + 12345u;
    int k = (int)((seed >> 8) % POOL), op = (int)((seed >> 20) % 3);
    if (op < 2) {
      CHECK(oa_insert(&t, names[k], &vals[step % 400]) == OA_OK);
      if (!in[k]) { in[k] = 1; n++; }
      model[k] = &vals[step % 400];
    } else {
      int had = in[k];
      CHECK(oa_remove(&t, names[k]) == (bool)had);
      if (had) { in[k] = 0; n--; model[k] = NULL; }
    }
    CHECK((int)t.count == n);
  }
  for (int k = 0; k < POOL; k++) CHECK(oa_get(&t, names[k]) == (in[k] ? model[k] : NULL));
  CHECK(occupied(&t) == n);
  oa_destroy(&t);
}

typedef struct { const char *name; void (*fn)(void); } test_t;
static const test_t tests[] = {
  {"insert and get", t_basic},
  {"update existing key", t_update},
  {"colliding keys probe forward", t_collide},
  {"probing wraps past the last slot", t_wrap},
  {"key is copied", t_key_copied},
  {"fill every slot", t_fill},
  {"insert into full table", t_full},
  {"update on full table", t_update_full},
  {"missing key on full table stops", t_missing_full},
  {"remove", t_remove},
  {"remove in the middle of a probe run", t_remove_chain},
  {"removed slot is reused", t_reuse},
  {"no duplicate past a removed slot", t_no_duplicate},
  {"full table with a removed slot", t_tomb_full},
  {"table of only removed slots", t_all_tombstones},
  {"6000 random operations vs a model", t_stress},
};

int main(void) {
  for (int i = 0; i < 512; i++) snprintf(names[i], sizeof names[i], "name%d", i);
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
