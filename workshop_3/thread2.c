#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct Number {
  int x;
  int y;
};

void *func2 (void *arg){
  struct Number* no = (struct Number*)arg;
  printf("x = %d, y = %d\n", no->x, no->y);
  pthread_exit(NULL);
}

int main() {
  pthread_t thread_id;
  struct Number* no;
  no = (struct Number*)malloc(sizeof(struct Number));
  no->x = 1;
  no->y = 2;
  pthread_create(&thread_id, NULL, func2,(void*)no);
  pthread_join(thread_id, NULL);
  return 0;
}

