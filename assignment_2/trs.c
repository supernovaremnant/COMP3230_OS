#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "trs.h"

int req[AGENT_NO][COL*ROW];		// the request array
int served_req[AGENT_NO][COL*ROW][3];		// served requests
rt *t;							// reservation table
sem_t reader_sem;				// semaphore for reading
sem_t writer_sem;				// semaphore for writing
int mode;

void *agent(void *);

void reserve(int req_id, int row, int col, int agent_id)  {

	int i;

	usleep(1);

	for (i = 0; i < req[agent_id][req_id]; i++)
		t->table[row][col+i] = req[agent_id][req_id];
	served_req[agent_id][req_id][0] = row;
	served_req[agent_id][req_id][1] = col;
	served_req[agent_id][req_id][2] = req[agent_id][req_id];
	t->seats[agent_id] += req[agent_id][req_id];

	return;
}

void withdraw(int req_id, int agent_id)  {

	int i;
	int del_row, del_col;	

	usleep(1);

	// has this request been withdrawn before?
	if (served_req[agent_id][(req[agent_id][req_id]*-1)][0] == -1)
		return;
	del_row = served_req[agent_id][req[agent_id][req_id]*-1][0];
	del_col = served_req[agent_id][req[agent_id][req_id]*-1][1];
	for (i = 0; i < served_req[agent_id][req[agent_id][req_id]*-1][2]; i++)
		t->table[del_row][del_col+i] = 0;

	// mark this request as withdrawn
	served_req[agent_id][req[agent_id][req_id]*-1][0] = -1;

	t->seats[agent_id] -= served_req[agent_id][req[agent_id][req_id]*-1][2];

	return;
}

// print out the whole table, use only if you have lowered ROW and COL in trs.h
void print_table(rt *t)  {

	int i, j;
	for (i = 0; i < ROW; i++)  {
	  for (j = 0; j < COL; j++)
		printf("%3d", t->table[i][j]);
	  printf("\n");
	}
	return;
}

// check the table consistency
int check_table(rt *t, int *vacant)  {

	int i, j, k, r;
	int seats_reserved = 0;

	*vacant = ROW*COL;

	for (i = 0; i < ROW; i++)
	  for (j = 0; j < COL; j++)  {
	    if ((r = t->table[i][j]) > 0)  {
	      for (k = 0; k < r; k++)
		if (j+k >= COL || t->table[i][j+k] != r)
			return 1;
	      j += r-1;
	      (*vacant) -= r;
	    }
	  }
	for (i = 0; i < AGENT_NO; i++)
		seats_reserved += t->seats[i];

	if (seats_reserved != COL*ROW - *vacant)
		return 1;

	return 0;
}

// initialize the reservation table
void init_rt(rt *t)  {

	int i, j;

	t->reader_count = 0;
	for (i = 0; i < ROW; i++)
		t->reader_counts[i] = 0;

	for (i = 0; i < ROW; i++)
		for (j = 0; j < COL; j++)
			t->table[i][j] = 0;
	for (i = 0; i < AGENT_NO; i++)
		t->seats[i] = 0;

}

int main(int argc, char **argv) {
   
   int i, j, k, status, seed;	// counters, status, and random seed
   int agent_id[AGENT_NO];	// agents' ids
   struct timeval tv1, tv2;	// timer
   int vacant, error;		// for checking table's consistency
   long usec;			// time used in microseconds
   float sec;			// time used in seconds
   int agent_finished = 0;	// number of agents finished
   int wd;			// withdrawal variable
   pthread_t agents[AGENT_NO];	//for thread creation

   // parse the parameters
   if (argc <= 2) {
	printf("Usage: %s [random_seed] [mode]\n", argv[0]);
	exit(0);
   }
   seed = (int)strtol(argv[1], NULL, 0);
   mode = (int)strtol(argv[2], NULL, 0);

   if (seed < 0)  {
	printf("Random seed must be non-negative\n");
	exit(0);
   }

   if (mode != 0 && mode != 1) {
	printf("\"mode\" must be either 0 or 1\n");
	exit(0);
   }

   // init the value of semaphores
   sem_init(&reader_sem, 0, 1);
   sem_init(&writer_sem, 0, 1);

   //set agent ids (0 to AGENT_NO-1)
   for (i=0; i<AGENT_NO; i++)
    agent_id[i] = i;

   // generate the reservation and withdraw requests
   srand(seed);
   for (i = 0; i < AGENT_NO; i++)
	for (j = 0; j < COL*ROW; j++)  {
	  wd = 1+(int) (WITHDRAW*1.0*rand()/(RAND_MAX+1.0));
	  if (!(wd % WITHDRAW) && j >= 1)
	  	req[i][j] = -1 * ((int)(j*1.0*rand()/(RAND_MAX+1.0)));
	  else
	  	req[i][j] = MIN_SEAT_REQ+(int)((MAX_SEAT_REQ-MIN_SEAT_REQ+1)*1.0*rand()/(RAND_MAX+1.0));
	}

   // initialize the array for storing served requests
   for(k =0; k < AGENT_NO; k++)
    for (i = 0; i < COL*ROW; i++)
	 for (j = 0; j < 3; j++) 
		served_req[k][i][j] = 0;

   // create the reservation table
   t = (rt *) malloc(sizeof(rt));

   // initialize the reservation table
   init_rt(t);

   // start timer, create 3 agents for doing reservations
   gettimeofday(&tv1, NULL);

   // create agent threads, pass id to each thread
   for(i=0; i<AGENT_NO; i++)
  	pthread_create(agents+i, NULL, agent, (void *)(agent_id+i));

   // wait for agents to terminate
   for(i=0; i<AGENT_NO; i++)
  	pthread_join(agents[i], NULL);

   gettimeofday(&tv2, NULL);
   usec = (tv2.tv_sec - tv1.tv_sec)*1000000 + (tv2.tv_usec - tv1.tv_usec);
   sec = (float)usec / (float)1000000;

   // check table's consistency
   error = check_table(t, &vacant);

   // printout 4 numbers: a b c d
   // a = 0 => table consistent; a = 1 => table inconsistent
   // b = computing time in microseconds
   // c = no. of seats remain vacant
   
   printf("%d %.4f %d\n", error, sec, vacant);

   printf("Agent Detail\n");

   for(i=0; i<AGENT_NO; i++){
	   usec = (t->end_time[i].tv_sec - t->start_time[i].tv_sec)*1000000 + (t->end_time[i].tv_usec - t->start_time[i].tv_usec);
       sec = (float)usec / (float)1000000;
	   printf("Agent%d %d %.4f\n", i, t->seats[i], sec);
   }
    print_table(t->table);
   return 0;
}
