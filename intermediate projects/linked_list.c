#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct list_node {
  void *data;
  struct list_node *next;
} list_node_t;

typedef struct {
  list_node_t *head;
  size_t count;
} list_t;

bool compare_uint8(const void *a, const void *b) {
  return *(const uint8_t *)a == *(const uint8_t *)b;
}

typedef bool (*compare_fn_t)(const void *a, const void *b);

void list_init(list_t *list) {
  list->head = NULL;
  list->count = 0;
}

void list_push_front(list_t *list, void *data) {
  list_node_t *new_list_node = malloc(sizeof(list_node_t));
  if (new_list_node == NULL) {
    printf("Heap FULL!!");
    return;
  }
  new_list_node->data = data;
  new_list_node->next = list->head;
  list->count++;
  list->head = new_list_node;
}

bool list_remove(list_t *list, void *data, compare_fn_t cmp) {
  list_node_t *prev_node = NULL;
  list_node_t *current_node = list->head;

  if (list->count == 0) {
    return false;
  }

  for (uint8_t i = 0; i < list->count; i++) {
    if (cmp(current_node->data, data)) {
      if (current_node->next == NULL && list->count == 1) {
        list->head = NULL;
        list->count = 0;
        free(current_node);
        return true;
      }
      if (list->head == current_node) {
        list->head = current_node->next;
        list->count--;
        free(current_node);
        return true;
      }
      prev_node->next = current_node->next;
      free(current_node);
      list->count--;
      return true;
    }
    prev_node = current_node;
    current_node = current_node->next;
  }
  return false;
}

list_node_t *list_find(list_t *list, void *data, compare_fn_t cmp) {
  list_node_t *current_node = list->head;
  if (list->count == 0) {
    return NULL;
  }
  for (uint8_t i = 0; i < list->count; i++) {
    if (cmp(current_node->data, data)) {
      return current_node;
    }
    if (current_node->next == NULL) {
      return NULL;
    }
    current_node = current_node->next;
  }
  return NULL;
}

void list_destroy(list_t *list) {
  if (list->count == 0) {
    printf("Liste already empty");
    return;
  }
  list_node_t *current_node = list->head;
  for (uint8_t i = 0; i < list->count; i++) {
    list_node_t *next_node = current_node->next;
    free(current_node);
    current_node = next_node;
  }
  list->count = 0;
  list->head = NULL;
}

int main() {
  list_t new_list;
  compare_fn_t comp_fun = compare_uint8;
  list_init(&new_list);
  printf("New list intialised!\n");
  uint8_t data[4] = {23, 10, 6, 89};
  uint8_t data_err = 10;
  uint8_t data_del = 89;
  uint8_t data_del2 = 23;
  for (uint8_t i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
    list_push_front(&new_list, &data[i]);
  }
  printf("Head node data: %d\n", *(uint8_t *)new_list.head->data);
  printf("Searching for data %d in list....\n", data_err);
  printf("%s found in list\n",
         list_find(&new_list, &data_err, comp_fun) != NULL ? "Yes" : "Not");
  printf("Removing data %d from list....\n", data_del);
  if (list_remove(&new_list, &data_del, comp_fun)) {
    printf("Data %d deleted from list\n", data_del);
  } else {
    printf("Data %d not found in the list", data_del);
  }
  printf("Searching for data %d in list....\n", data_err);
  printf("%s found in list\n",
         list_find(&new_list, &data_err, comp_fun) != NULL ? "Yes" : "Not");
  (void)list_remove(&new_list, &data_del2, comp_fun);
  uint8_t new_data = 74;
  list_push_front(&new_list, &new_data);
  printf("Complete List: \n");
  list_node_t *current = new_list.head;
  for (uint8_t i = 0; i < new_list.count; i++) {
    printf("%d\n", *(uint8_t *)current->data);
    current = current->next;
  }
  printf("New head node data: %d\n", *(uint8_t *)new_list.head->data);
  return 0;
}
