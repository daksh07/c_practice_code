#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define HEAP_SIZE 4096

typedef struct block_header {
  size_t size;
  int free;
  struct block_header *next;
} block_header_t;

static uint8_t heap[HEAP_SIZE];
static block_header_t *free_list = NULL;

void heap_init(void) {
  free_list = (block_header_t *)heap;
  free_list->size = HEAP_SIZE - sizeof(block_header_t);
  free_list->free = 1;
  free_list->next = NULL;
}

void *my_malloc(size_t size) {
  block_header_t *current = free_list;
  while (current != NULL) {
    if (current->free == 1 && current->size >= size) {
      break;
    }
    current = current->next;
  }

  if (current == NULL) {
    return NULL;
  }

  size_t aligned_size =
      (size + 7) & ~((size_t)7); // round size up to nearest multiple of 8 for
                                 // clean memory access
  size_t leftover = current->size - aligned_size;

  if (leftover > sizeof(block_header_t) + 16) {
    block_header_t *new_block =
        (block_header_t *)((uint8_t *)current + sizeof(block_header_t) +
                           aligned_size);
    new_block->size = leftover - sizeof(block_header_t);
    new_block->free = 1;
    new_block->next = current->next;
    current->size = aligned_size;
    current->next = new_block;
  }

  current->free = 0;
  return (uint8_t *)(current + 1);
}

void my_free(void *ptr) {

  if (ptr == NULL) {
    return;
  }

  block_header_t *block = (block_header_t *)ptr - 1;
  block->free = 1;

  if (block->next != NULL && block->next->free == 1) {
    block->size += sizeof(block_header_t) + block->next->size;
    block->next = block->next->next;
  }

  block_header_t *prev = NULL;
  block_header_t *current = free_list;
  // loop through every block in the list and as soon as you reach the current
  // block stop example head->n1->n2->block->n3 so the loop stops at block and
  // prev hold the pointer to n2
  while (current != NULL && current != block) {
    prev = current;
    current = current->next;
  }

  if (prev != NULL && prev->free == 1) {
    prev->size += sizeof(block_header_t) + block->size;
    prev->next = block->next;
  }
}

int main() {
  heap_init();

  uint8_t *mallocB = my_malloc(500);
  uint8_t *mallocA = my_malloc(350);
  uint8_t *mallocC = my_malloc(5000);

  printf("Total heap size: %lu\n", sizeof(heap));
  printf("Size of Malloc B: %zu\n", ((block_header_t *)mallocB - 1)->size);
  printf("Is mallocB free? %s\n",
         ((block_header_t *)mallocB - 1)->free == 1 ? "Yes" : "No");
  printf("Size of Malloc A: %zu\n", ((block_header_t *)mallocA - 1)->size);
  printf("Is mallocA free? %s\n",
         ((block_header_t *)mallocA - 1)->free == 1 ? "Yes" : "No");
  printf("Freeing mallocB....\n");
  my_free(mallocB);
  printf("mallocB freed. \n");
  printf("Freeing mallocA....\n");
  my_free(mallocA);
  printf("mallocA freed.\n");
  printf("Size of Malloc A: %zu\n", ((block_header_t *)mallocA - 1)->size);
  printf("Is mallocA free? %s\n",
         ((block_header_t *)mallocA - 1)->free == 1 ? "Yes" : "No");
  printf("Size of Malloc B: %zu\n", ((block_header_t *)mallocB - 1)->size);
  printf("Is mallocB free? %s\n",
         ((block_header_t *)mallocB - 1)->free == 1 ? "Yes" : "No");
  printf("Pointer to mallocC %p\n", (void *)mallocC);
}
