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

// Hash function (djb2) — already implemented for you.
static unsigned long hash_string(const char *key) {
  unsigned long hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + (unsigned long)c;
  }
  return hash;
}

/*
 * First we intialise number of pointers = capacity of bucket
 * The size of every pointer is equal to the pointer size needed to point to a
 * node Then we update the capaticty of the hash table We intialise the count of
 * the hash table
 * */
void ht_init(hash_table_t *ht, size_t capacity) {
  ht->capacity = capacity;
  ht->buckets = calloc(ht->capacity, sizeof(hash_node_t *));
  ht->count = 0;
  printf("Hash table intialisation complete.\n");
}

/*
 * Goes through every node in every bucket index
 * prints the key and value
 * */
void print_hash_table(hash_table_t *ht) {
  if (ht->capacity == 0) {
    printf("Error! Hash table empty.\n");
    return;
  }
  printf("key : value\n");
  for (size_t i = 0; i < ht->capacity; i++) {
    for (hash_node_t *n = ht->buckets[i]; n != NULL; n = n->next) {
      printf("%s : %s\n", n->key, (char *)n->value);
    }
  }
}

/*
 * First we calculate the index for new data
 * This index needs to be within the capacity of the bucker hence % capacity
 * Within that index we check all available nodes first if any node has the same
 * key If it has the same key then just update the value and return true If
 * there is no entry with that key, we create a new node pointer pointind to
 * actual node *node = malloc(sizeof(hash_node_t)) here basically malloc
 * allocates a memory block for a new node which is the same size of the node
 * structure and then returns a pointer to that memory block for the new node
 * then we allocate memory for the key, size is equal to the string length of
 * the key input + 1 for null character we copy the data key to the memory for
 * key we copy the value to node value then we need to make sure that the
 * bucket[index] is always pointing to the latest entry so we make the new node
 * point to the old node bucket[index] was point to by node->next =
 * bucket[index] and we make the bucket[index] to point to the latest node by
 * bucket[index] = node and we increase the count in the hash table
 * */

bool ht_insert(hash_table_t *ht, const char *key, void *value) {
  size_t index = hash_string(key) % ht->capacity;
  for (hash_node_t *n = ht->buckets[index]; n; n = n->next) {
    if (strcmp(n->key, key) == 0) {
      n->value = value;
      return true;
    }
  }
  hash_node_t *node = malloc(sizeof(hash_node_t));
  node->key = malloc(strlen(key) + 1);
  if (node == NULL || node->key == NULL) {
    printf("Error! Memory allocation failed.\n");
    return false;
  }
  strcpy(node->key, key);
  node->value = value;
  node->next = ht->buckets[index];
  ht->buckets[index] = node;
  ht->count++;
  return true;
}

/*
 * First calculate the bucket index
 * Then check for the key in every node in that bucket
 * if the key matches reture the key value
 * */
void *ht_get(hash_table_t *ht, const char *key) {
  size_t index = hash_string(key) % ht->capacity;
  for (hash_node_t *n = ht->buckets[index]; n != NULL; n = n->next) {
    if (strcmp(n->key, key) == 0) {
      return n->value;
    }
  }
  return NULL;
}

/*
 * First we calculate the bucket index
 * Then we go through every node in the bucket to check for the key
 * if the key matches, if prev_node = NULL means this is the first node in the
 * bucket hence we update the link from bucket[index] to the not next to current
 * node node->next if this prev_node != NULL means this is not the first node
 * hence we update the previous node link to node->next then we delete the key
 * and the node
 * */
bool ht_remove(hash_table_t *ht, const char *key) {
  size_t index = hash_string(key) % ht->capacity;
  hash_node_t *prev_node = NULL;
  for (hash_node_t *n = ht->buckets[index]; n != NULL;
       prev_node = n, n = n->next) {
    if (strcmp(n->key, key) == 0) {
      if (prev_node != NULL) {
        prev_node->next = n->next;
      } else {
        ht->buckets[index] = n->next;
      }
      printf("Deleting entry....\n");
      free(n->key);
      free(n);
      ht->count--;
      printf("Entry deleted. \n");
      return true;
    }
  }
  return false;
}

/*
 * Goes through every node in every bucket
 * saves the next node address before Deleting
 * deletes the key and node
 *
 * */

void ht_destroy(hash_table_t *ht) {
  hash_node_t *next_node = NULL;
  for (size_t i = 0; i < ht->capacity; i++) {
    for (hash_node_t *n = ht->buckets[i]; n != NULL; n = next_node) {
      next_node = n->next;
      free(n->key);
      free(n);
    }
  }
  free(ht->buckets);
  ht->count = 0;
  ht->capacity = 0;
  ht->buckets = NULL;
}

int main(void) {
  hash_table_t ht;
  ht_init(&ht, INITIAL_CAPACITY);
  ht_insert(&ht, "name", "rohan");
  ht_insert(&ht, "city", "delhi");
  print_hash_table(&ht);
  ht_remove(&ht, "name");
  print_hash_table(&ht);
  char *value = ht_get(&ht, "city");
  printf("Testting get value: %s\n", value == NULL ? "Not found" : value);
  //  printf("Size of node: %lu\n", sizeof(hash_node_t));
  //  printf("Size of node pointer: %lu\n", sizeof(hash_node_t *));
  ht_destroy(&ht);
  return 0;
}
