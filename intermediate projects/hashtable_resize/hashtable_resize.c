#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * String-keyed chained hash table, now with automatic resizing.
 *
 * Why resize: with a fixed number of buckets, chains get longer as you add
 * entries and lookups slow down. Keeping the table at most 3/4 full (on
 * average, at most 3/4 of an entry per bucket) keeps the chains short.
 *
 * Rules:
 *   - After a NEW key is added, if count * 4 > capacity * 3, the table
 *     doubles its capacity. (16 buckets: the 13th key triggers 16 -> 32.)
 *   - Updating an existing key never changes count and never resizes.
 *   - Removing keys never shrinks the table.
 *   - ht_resize(ht, new_capacity) can also be called directly, with any
 *     new_capacity >= 1 (bigger or smaller). It must not allocate new nodes or
 *     copy keys: the existing nodes are moved into the new bucket array.
 *   - If ht_resize fails (allocation fails, or new_capacity is 0) the table is
 *     left exactly as it was.
 */

#define INITIAL_CAPACITY 16

typedef struct hash_node {
  char *key;
  void *value;
  struct hash_node *next;
} hash_node_t;

typedef struct {
  hash_node_t **buckets;
  size_t capacity;
  size_t count;
} hash_table_t;

static unsigned long hash_string(const char *key) {
  unsigned long hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + (unsigned long)c;
  }
  return hash;
}

void ht_init(hash_table_t *ht, size_t capacity) {
  ht->buckets = calloc(capacity, sizeof(hash_node_t *));
  ht->capacity = capacity;
  ht->count = 0;
}

// Move every node into a bucket array of new_capacity buckets.
bool ht_resize(hash_table_t *ht, size_t new_capacity) {
  // TODO: allocate the new bucket array (all slots NULL). If that fails, or
  // new_capacity is 0, return false without touching the table.
  // TODO: walk every old chain and re-link each node into the new array. The
  // bucket index depends on the capacity, so work it out again.
  // TODO: free the old bucket array, switch ht to the new one, update the
  // capacity. count does not change.
  return false;
}

bool ht_insert(hash_table_t *ht, const char *key, void *value) {
  size_t index = hash_string(key) % ht->capacity;
  for (hash_node_t *n = ht->buckets[index]; n != NULL; n = n->next) {
    if (strcmp(n->key, key) == 0) {
      n->value = value;
      return true;
    }
  }
  hash_node_t *node = malloc(sizeof(hash_node_t));
  if (node == NULL) return false;
  node->key = malloc(strlen(key) + 1);
  if (node->key == NULL) {
    free(node);
    return false;
  }
  strcpy(node->key, key);
  node->value = value;
  node->next = ht->buckets[index];
  ht->buckets[index] = node;
  ht->count++;

  // TODO: if the table is now too full, double its capacity. If the resize
  // fails the new entry is still stored, so this function still returns true.
  return true;
}

void *ht_get(hash_table_t *ht, const char *key) {
  size_t index = hash_string(key) % ht->capacity;
  for (hash_node_t *n = ht->buckets[index]; n != NULL; n = n->next) {
    if (strcmp(n->key, key) == 0) return n->value;
  }
  return NULL;
}

bool ht_remove(hash_table_t *ht, const char *key) {
  size_t index = hash_string(key) % ht->capacity;
  hash_node_t *prev = NULL;
  for (hash_node_t *n = ht->buckets[index]; n != NULL; prev = n, n = n->next) {
    if (strcmp(n->key, key) == 0) {
      if (prev == NULL) ht->buckets[index] = n->next;
      else prev->next = n->next;
      free(n->key);
      free(n);
      ht->count--;
      return true;
    }
  }
  return false;
}

void ht_destroy(hash_table_t *ht) {
  for (size_t i = 0; i < ht->capacity; i++) {
    hash_node_t *n = ht->buckets[i];
    while (n != NULL) {
      hash_node_t *next = n->next;
      free(n->key);
      free(n);
      n = next;
    }
  }
  free(ht->buckets);
  ht->buckets = NULL;
  ht->capacity = 0;
  ht->count = 0;
}

int main(void) {
  hash_table_t ht;
  ht_init(&ht, 4); // deliberately tiny, so it has to grow
  printf("start:  capacity %zu, count %zu\n", ht.capacity, ht.count);

  char keys[20][8];
  for (int i = 0; i < 20; i++) {
    snprintf(keys[i], sizeof keys[i], "key%d", i);
    ht_insert(&ht, keys[i], keys[i]);
  }
  printf("after 20 inserts: capacity %zu, count %zu\n", ht.capacity, ht.count);

  int found = 0;
  for (int i = 0; i < 20; i++)
    if (ht_get(&ht, keys[i]) == keys[i]) found++;
  printf("found %d of 20\n", found);

  ht_destroy(&ht);
  return 0;
}
