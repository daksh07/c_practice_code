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
  size_t tombstones;
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
  // TODO: if the new key reuses a tombstone slot, tombstones goes down by 1.
  // TODO: after storing a NEW key, check the load rules and call the resize.
  // A failed resize does not change the return value.
  /*
   * Using the while loop we are going through the table
   * Starting from the calculated probing index
   * if we find an OCCUPIED slot with the same key, we update the value
   * else when we find the first tombstone we capture it's index
   * after going through the whole table
   * if there is no tombstone but there is an empty slot we write to that slot
   * else we write to the tombstone
   * if the count = capacity and there is no tombstone we return table full
   * */
  if (t->capacity == 0) return OA_ERR_FULL;
  size_t index = hash_string(key) % t->capacity;
  size_t index_count = index;
  size_t count = 0;
  size_t tsn_idx = t->capacity;
  while (t->slots[index_count].state != SLOT_EMPTY && count < t->capacity){
    if (t->slots[index_count].state == SLOT_OCCUPIED){
      if (strcmp(t->slots[index_count].key, key) == 0){
        t->slots[index_count].value = value;
        return OA_OK;
      }
    }
    if (t->slots[index_count].state == SLOT_TOMBSTONE && tsn_idx == t->capacity){
      tsn_idx = index_count;
    }
    index_count = (index_count + 1) % t->capacity;
    count++;
  }
  if (count == t->capacity && tsn_idx == t->capacity) return OA_ERR_FULL;
  if (t->slots[index_count].state == SLOT_EMPTY && tsn_idx == t->capacity){
    tsn_idx = index_count;
  }
  char *new_key = malloc(sizeof(char) * (strlen(key) + 1));
  if (new_key == NULL) return OA_ERR_NOMEM;
  t->slots[tsn_idx].key = new_key;
  strcpy(t->slots[tsn_idx].key, key);
  t->slots[tsn_idx].value = value;
  t->slots[tsn_idx].state = SLOT_OCCUPIED;
  t->count++;
  return OA_OK;
}

/*
 * Goes through the table
 * Looks for occupied slot, if key matches returns the value
 * */
void *oa_get(oa_table_t *t, const char *key) {
  // TODO: probe from the home slot. Return NULL if the key is not there.
  if (t->capacity == 0) return NULL;
  size_t index = hash_string(key) % t->capacity;
  size_t count = 0;
  while(t->slots[index].state != SLOT_EMPTY && count < t->capacity){
    if (t->slots[index].state == SLOT_OCCUPIED){
      if (strcmp(t->slots[index].key, key) == 0){
        return t->slots[index].value;
      }
    }
    index = (index + 1) % t->capacity;
    count++;
  }
  return NULL;
}

/*
 * Goes through the table
 * if the slot at index is empty returns false
 * empty means there is nothing there and table finishes here
 * tombstone means something was there but was deleted
 * and the table continues after tombstone
 * if the slot is occupied, check the key
 * if the key matches, free the key, Nullify the key pointer
 * decrease the count and return true
 * */
bool oa_remove(oa_table_t *t, const char *key) {
// TODO: tombstones goes up by 1 when a slot becomes a tombstone.

  if (t->capacity == 0) return false;
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

// Put an already-allocated key into a slot array. The key is NOT copied.
// Returns false if no slot is free. Used only while rebuilding.
static bool oa_place(oa_slot_t *slots, size_t capacity, char *key, void *value) {
  // TODO: probe from the home slot for this capacity, store key/value/state.
}

// Rebuild the table with new_capacity slots.
bool oa_resize(oa_table_t *t, size_t new_capacity) {
  // TODO: return false for new_capacity == 0 or new_capacity < t->count.
  // TODO: allocate the new slot array (all EMPTY). On failure, leave t alone.
  // TODO: move every OCCUPIED slot into the new array with oa_place.
  //       Tombstones and empty slots are not moved.
  // TODO: free the old slot array, switch t to the new one, update capacity,
  //       set tombstones to 0. count is unchanged.
}

void oa_destroy(oa_table_t *t) {
// TODO: reset tombstones as well.
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
  printf("key at slot 0: %s\n",t.slots[0].key);
  for (int i = 0; i < 6; i++) {
    int *v = oa_get(&t, names[i]);
    printf("%-8s home %zu -> %s\n", names[i], hash_string(names[i]) % t.capacity,
           v ? "found" : "MISSING");
  }
  oa_remove(&t, "blue");
  printf("blue after remove: %s\n", oa_get(&t, "blue") ? "found" : "gone");
  printf("count after remove: %zu\n", t.count);

  oa_destroy(&t);
  return 0;
}
