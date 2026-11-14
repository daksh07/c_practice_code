#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * String-keyed hash table with OPEN ADDRESSING (linear probing).
 *
 * There are no linked lists. Every entry lives directly in one flat array of
 * slots. Each key has a "home" slot:
 *
 *     home = hash_string(key) % capacity
 *
 * If the home slot is taken by another key, try home+1, home+2, ... and wrap
 * around to slot 0 after the last slot. This is called probing.
 *
 *   capacity 8, keys A and B both have home 5, C has home 6:
 *       insert A -> slot 5        insert B -> slot 6 (5 is taken)
 *       insert C -> slot 7 (6 is taken, even though it is not C's own chain)
 *
 * Each slot is in one of three states:
 *   SLOT_EMPTY      never used
 *   SLOT_OCCUPIED   holds a key and a value
 *   SLOT_TOMBSTONE  used to hold a key that was removed
 *
 * Rules:
 *   - The capacity is fixed when the table is created. There is no resizing.
 *     The table can be completely full (count == capacity).
 *   - The table keeps its own copy of every key. Values are stored by
 *     reference.
 *   - Inserting a key that already exists replaces its value and returns
 *     OA_OK. count does not change. This works even when the table is full.
 *   - Inserting a new key into a table with no usable slot returns
 *     OA_ERR_FULL and changes nothing.
 *   - A lookup stops when it reaches an EMPTY slot, or after it has looked at
 *     every slot once.
 *   - A removed slot must still let lookups continue past it, and a later
 *     insert may reuse it.
 */

typedef enum {
  SLOT_EMPTY = 0, // calloc gives you this
  SLOT_OCCUPIED,
  SLOT_TOMBSTONE,
} slot_state_t;

typedef struct {
  char *key;
  void *value;
  slot_state_t state;
} oa_slot_t;

typedef struct {
  oa_slot_t *slots;
  size_t capacity;
  size_t count; // number of OCCUPIED slots
} oa_table_t;

typedef enum {
  OA_OK,
  OA_ERR_FULL,
  OA_ERR_NOMEM,
} oa_status_t;

static unsigned long hash_string(const char *key) {
  unsigned long hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + (unsigned long)c;
  }
  return hash;
}

bool oa_init(oa_table_t *t, size_t capacity) {
  t->slots = calloc(capacity, sizeof(oa_slot_t));
  t->capacity = t->slots ? capacity : 0;
  t->count = 0;
  return t->slots != NULL;
}

oa_status_t oa_insert(oa_table_t *t, const char *key, void *value) {
  // TODO: probe from the home slot, looking at no more than `capacity` slots.
  // TODO: update the value if the key is already in the table.
  // TODO: otherwise store a copy of the key in the best free slot.
  if(t->count >= t->capacity) return OA_ERR_FULL;
  size_t index = hash_string(key) % t->capacity;
  for (size_t i = 0; i < t->capacity; i++){
    if(t->slots[index].state == SLOT_OCCUPIED){
      if (strcmp(t->slots[index].key, key) == 0){
        t->slots[index].value = value;
        return OA_OK;
      } else {
          index = (index + 1) % t->capacity;
      }
    }
    if(t->slots[index].state == SLOT_EMPTY || t->slots[index].state == SLOT_TOMBSTONE){
      char *new_key = malloc(sizeof(char)*(strlen(key) + 1));
      strcpy(new_key, key);
      t->slots[index].key = new_key;
      t->slots[index].value = value;
      t->slots[index].state = SLOT_OCCUPIED;
      t->count++;
      return OA_OK;
    }
  }
  return OA_ERR_FULL;
}

void *oa_get(oa_table_t *t, const char *key) {
  // TODO: probe from the home slot. Return NULL if the key is not there.
  size_t index = hash_string(key) % t->capacity;
  size_t count = 0;
  while(t->slots[index].state != SLOT_EMPTY || count < t->count){
    if (strcmp(t->slots[index].key, key) == 0){
      return t->slots[index].value;
    }
    index = (index + 1) % t->capacity;
    count++;
  }
  (void)t; (void)key;
  return NULL;
}

bool oa_remove(oa_table_t *t, const char *key) {
  // TODO: find the key, free it, and leave the slot in the right state.
  if (key == NULL) return false;
  size_t index = hash_string(key) % t->capacity;
  for(size_t i = 0; i < t->capacity; i++){
    if (t->slots[index].state == SLOT_EMPTY){
      return false;
    }
    if (t->slots[index].state == SLOT_OCCUPIED){
      if (strcmp(t->slots[index].key, key) == 0){
        free(t->slots[index].key);
        t->slots[index].key = NULL;
        t->slots[index].state = SLOT_TOMBSTONE;
        t->count--;
        return true;
      }
    }
    index = (index + 1) % t->capacity;
  }
  return false;
}

void oa_destroy(oa_table_t *t) {
  for (size_t i = 0; i < t->capacity; i++) {
    if (t->slots[i].state == SLOT_OCCUPIED) free(t->slots[i].key);
  }
  free(t->slots);
  t->slots = NULL;
  t->capacity = 0;
  t->count = 0;
}

int main(void) {
  oa_table_t t;
  oa_init(&t, 8);

  const char *names[] = {"red", "green", "blue", "cyan", "magenta", "yellow"};
  int values[] = {1, 2, 3, 4, 5, 6};
  for (int i = 0; i < 6; i++) oa_insert(&t, names[i], &values[i]);
  printf("count after 6 inserts: %zu\n", t.count);

  for (int i = 0; i < 6; i++) {
    int *v = oa_get(&t, names[i]);
    printf("%-8s home %zu -> %s\n", names[i], hash_string(names[i]) % t.capacity,
           v ? "found" : "MISSING");
  }
  oa_remove(&t, "blue");
  printf("blue after remove: %s\n", oa_get(&t, "blue") ? "found" : "gone");
  printf("count after remove: %zu\n", t.count);

  //oa_destroy(&t);
  return 0;
}
