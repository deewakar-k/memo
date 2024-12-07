#include <unistd.h>
#include <stddef.h>
#include <pthread.h>

typedef char ALIGN[16] ;

union header {
  struct {
    size_t size;
    unsigned is_free;
    union header *next;
  } s;

  /* forcing the header to be aligned*/
  ALIGN stub;
};

typedef union header header_t;

header_t *head = NULL, *tail = NULL;
pthread_mutex_t global_malloc_lock;

header_t *get_free_block(size_t size) {
  header_t *curr = head;
  while (curr) {
    if (curr -> s.is_free && curr -> s.size >= size)
      return curr;
    curr = curr -> s.next;
  }
  return  NULL;
}

void *malloc(size_t size) {

  size_t total_size;
  void *block;
  header_t *header;

  if (!size) 
    return NULL;
  /* apply lock*/
  pthread_mutex_lock(&global_malloc_lock);
  header = get_free_block(size);
  if (header) {
    header -> s.is_free = 0;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header + 1);
  }
  /* if no free block, extend heap*/
  total_size = sizeof(header_t) + size;
  block = sbrk(total_size);
  if (block == (void*) - 1) {
    pthread_mutex_unlock(&global_malloc_lock);
    return NULL;
  }
  
  header = block;
  header -> s.size = size;
  header -> s.is_free = 0;
  header -> s.next = NULL;
  if (!head)
    head = header;
  if (tail)
    tail -> s.next = header;
  tail = header;
  pthread_mutex_unlock(&global_malloc_lock);

  return (void*)(header + 1);
}
