#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define TOTAL_COUNT 10000000

int count = 0;
sem_t sem;

void *func(int id){
	int i, tmp;
	for(i = 0; i < TOTAL_COUNT; i++){
		sem_wait(&sem);
		tmp = ++count;
		if(tmp % 2500000 == 0)
			printf("Thread %d: current count %d \n", id, tmp );
		sem_post(&sem);
	}
return;
}

int main(){

	pthread_t thread1, thread2, thread3;
	   
	int sem_init_val = 1;  
	int return_status = sem_init(&sem, 0, sem_init_val);
	int sem_val = -1;
	sem_getvalue(&sem, &sem_val);

	if(return_status == -1 ){
		printf("error creating semaphore\n");
		exit(1);
	}

	if(pthread_create(&thread1, NULL, &func, 1 )){
		printf("ERROR creating thread 1\n");
		exit(1);
	}  

	if(pthread_create(&thread2, NULL, &func, 2 )){
		printf("ERROR creating thread 2\n");
		exit(1);
	}
	if(pthread_create(&thread3, NULL, &func, 3 )){
		printf("ERROR creating thread 2\n");
		exit(1);
	}

	// wait for the completion of all threads

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	pthread_join(thread3, NULL);

	if (count != 3 * TOTAL_COUNT)
		printf("Error! current count is %d. total count should be %d\n", count, 2 * TOTAL_COUNT);
	else
		printf("OK. current count is %d\n", count);
	sem_destroy(&sem);	
	return 0;
}
