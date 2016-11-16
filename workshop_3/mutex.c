#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define TOTAL_COUNT 10000000

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int count = 0;

void *func()
{
  int i, tmp;

  for(i = 0; i < TOTAL_COUNT; i++)
  {
    pthread_mutex_lock(&mutex1);
    tmp = ++count;
    pthread_mutex_unlock(&mutex1);
  }

  pthread_exit(0);
}

int main()
{
  pthread_t thread1, thread2;
  
  if(pthread_create(&thread1, NULL, &func, NULL))
  {
    printf("ERROR creating thread 1\n");
    exit(1);
  }  
  if(pthread_create(&thread2, NULL, &func, NULL))
  {
    printf("ERROR creating thread 2\n");
    exit(1);
  }

  // wait for the completion of all threads
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  if (count != 2 * TOTAL_COUNT)
    printf("Error! current count is %d. total count should be %d\n", count, 2 * TOTAL_COUNT);
  else
    printf("OK. current count is %d\n", count);
  
  return 0;
}
