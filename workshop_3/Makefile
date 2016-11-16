all: thread1 thread2 mutex conVar semaphore

thread1: thread1.c
	gcc thread1.c -lpthread -o thread1 -Wall

thread2: thread2.c
	gcc thread2.c -lpthread -o thread2 -Wall

mutex: mutex.c
	gcc mutex.c -lpthread -o mutex -Wall

conVar: conVar.c
	gcc conVar.c -lpthread -o conVar

semaphore: semaphore.c
	gcc semaphore.c -lpthread -o sem -Wall

clean:
	rm -f thread1 thread2 mutex conVar sem
