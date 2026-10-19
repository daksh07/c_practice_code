#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void ht_destroy(hash_table_t *ht) {
  for (uint8_t i = 0; i < ht->capacity; i++) {
    for (hash_node_t *n = ht->buckets[i]; n != NULL; n = n->next) {
      free(n->key);
      free(n);
    }
  }
  free(ht->buckets);
  ht->capacity = 0;
}

// Hash function (djb2) — already implemented for you.
static unsigned long hash_string(const char *key) {
  unsigned long hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + (unsigned long)c;
  }
  return hash;
}

void print_hash_table(hash_table_t *ht) {
  if (ht->capacity == 0) {
    printf("Err! Hash table is empty.\n");
  } else {
    printf("Bucket size: %lu\n", ht->count);
    printf("key : value\n");
    for (uint8_t i = 0; i < ht->capacity; i++) {
      for (hash_node_t *node = ht->buckets[i]; node != NULL;
           node = node->next) {
        printf("%s : %s\n", node->key, (char *)node->value);
      }
    }
  }
}

void ht_init(hash_table_t *ht, size_t capacity) {
  // TODO: allocate ht->buckets as an array of `capacity` hash_node_t*
  // pointers (malloc/calloc). Think about why calloc might matter
  // here more than for other structs you've built.
  // TODO: set ht->capacity and ht->count.
  ht->buckets = calloc(capacity, sizeof(hash_node_t *));
  ht->capacity = capacity;
  ht->count = 0;
}

bool ht_insert(hash_table_t *ht, const char *key, void *value) {
  // TODO: compute the bucket index from hash_string(key) % ht->capacity.
  // TODO: walk that bucket's chain — if a node with this key already
  // exists, decide what should happen (update value? reject? your call,
  // but be deliberate about it).
  // TODO: otherwise, allocate a new hash_node_t, duplicate the key
  // (don't just store the caller's pointer — think about why), set
  // value, and insert it into the bucket's chain.
  // TODO: increment ht->count, return true/false to indicate success.
  size_t index = hash_string(key) % ht->capacity;

  for (hash_node_t *n = ht->buckets[index]; n != NULL; n = n->next) {
    if (strcmp(n->key, key) == 0) {
      n->value = value;
      return true;
    }
  }

  hash_node_t *node = malloc(sizeof(hash_node_t));
  if (node == NULL)
    return false;
  node->key = malloc(sizeof(key + 1));
  if (node->key == NULL) {
    free(node);
    return false;
  }
  strcpy(node->key, key);

  node->value = value;
  node->next = ht->buckets[index];
  ht->buckets[index] = node;
  ht->count++;
  return true;
}

void *ht_get(hash_table_t *ht, const char *key) {
  // TODO: compute the bucket index the same way as ht_insert.
  // TODO: walk that bucket's chain comparing keys (strcmp, not ==).
  // TODO: return the value if found, NULL otherwise.
  hash_node_t *node_entry = *(ht->buckets);
  for (uint8_t i = 0; i < ht->capacity; ht++) {
    if (strcmp(node_entry->key, key) == 0) {
      printf("Entry found!\n");
      printf("key: %s\n", node_entry->key);
      printf("value: %s\n", (char *)node_entry->value);
      return node_entry->value;
    }
  }
  return NULL;
}

bool ht_remove(hash_table_t *ht, const char *key) {
  // TODO: compute the bucket index.
  // TODO: walk the chain with a prev/current pair (same pattern as
  // your linked list's list_remove) to unlink the matching node.
  // TODO: free the node's duplicated key and the node itself.
  // Decide: should this also free `value`? Who owns that memory?
  // TODO: decrement ht->count, return true/false.
  size_t index = hash_string(key) % ht->capacity;
  hash_node_t *prev = NULL;
  for (hash_node_t *n = ht->buckets[index]; n != NULL; prev = n, n = n->next) {
    if (strcmp(key, n->key) == 0) {
      if (prev == NULL) {
        ht->buckets[index] = n->next;
      } else {
        prev->next = n->next;
      }
      free(n->key);
      free(n);
      ht->count--;
      return true;
    }
  }
  return false;
}

int main(void) {
  hash_table_t ht;
  ht_init(&ht, INITIAL_CAPACITY);
  char *key[] = {"rohan", "krishna", "city"};
  char *value[] = {"engineer", "student", "delhi"};
  for (uint8_t i = 0; i < 3; i++) {
    ht_insert(&ht, key[i], (void *)value[i]);
  }
  print_hash_table(&ht);
  ht_remove(&ht, "rohan");
  print_hash_table(&ht);
  ht_destroy(&ht);
  print_hash_table(&ht);
  return 0;
}
