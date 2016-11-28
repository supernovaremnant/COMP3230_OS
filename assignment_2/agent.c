#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "trs.h"

#define DEBUG 1

extern int req[AGENT_NO][COL*ROW];
extern int served_req[AGENT_NO][COL*ROW][3];		// served requests
extern rt *t;							// reservation table
extern sem_t reader_sem;				// semaphore for reading
extern sem_t writer_sem;				// semaphore for writing
extern int mode;

// you have to insert your code inside this file
// It may call other functions written by you, if there is any.
// if "mode" is 0, agent() should use the first-fit algorithm
// if "mode" is 1, agent() should use the best-fit algorithm 

void first_fit(int request_id, int agent_id, int request_seat);

void *agent(void *data)  {

    int id = *((int *) data);

    gettimeofday((t->start_time)+id, NULL);

    int i;
    int request; //how many seats does the agent request? 

    for(i=0; i<COL*ROW; i++){
        request = req[id][i];

        if(request <= 0){
            // Withdraw request
            // This part does not differ in Mode 0 and Mode 1
            
            //writer begin
            sem_wait( &writer_sem ); //lock writer
            withdraw(i, id);
            sem_post(&writer_sem);//unlock writer
            //writer end
        }
        else{
            // Reservation request
            // Need to first read the reservation table to look for potential seats
            // Double-check when you get the writing permission
            // If the seats are gone, keep the double_checked flag as 0
            // This request will be processed again;
            // But if none of the rows can satisfy this request, the thread should terminate immediately
            
            int double_checked = 0;
            
            while( !double_checked ){
                // Read table (need to take different actions for Mode 0 and Mode 1)
                // You may declare functions as you like
                
                int available_row = -1;
                int available_col = -1;
                int row_i = 0;
                
                //reader begin
                sem_wait( &reader_sem ); //lock reader
                t->reader_count += 1;
                if (t->reader_count == 1) {
                    sem_wait(&writer_sem);
                }
                sem_post(&reader_sem);
                
                do{
                    available_col = row_check(row_i, request);
                    if( available_col != -1 ){
                        available_row = row_i;
                    }
                    row_i ++;
                }while( row_i < ROW && available_col == -1 );
                
                sem_wait(&reader_sem);
                t->reader_count -= 1 ;
                
                if (t->reader_count == 0) {
                    sem_post(&writer_sem);
                }
                sem_post(&reader_sem);
                //end of reader
                
                //using data
                if( available_col == -1 ){
                    if(DEBUG)
                        printf("cannot find ava seats, end\n");
                    return -1;
                }else{
                    printf("ready to write %d, %d, %d \n", available_row, available_col, request);
                    //writer begin
                    sem_wait(&writer_sem);
                    
                    //check again, reader begin
                    int available_row_2 = -1; int available_col_2 = -1; row_i = 0;
                    
                    sem_wait( &reader_sem ); //lock reader
                    t->reader_count += 1;
                    if (t->reader_count == 1) {
                        sem_wait(&writer_sem);
                    }
                    sem_post(&reader_sem);
                    
                    do{
                        available_col_2 = row_check(row_i, request);
                        if( available_col_2 != -1 ){
                            available_row_2 = row_i;
                        }
                        row_i ++;
                    }while( row_i < ROW && available_col_2 == -1 );
                    
                    sem_wait(&reader_sem);
                    t->reader_count -= 1 ;
                    
                    if (t->reader_count == 0) {
                        sem_post(&writer_sem);
                    }
                    sem_post(&reader_sem);
                    //reader end
                    
                    //use reader data
                    if( available_col_2 == -1 ){
                        if(DEBUG)
                            printf("cannot find ava seats 2, end\n");
                        sem_post(&writer_sem);
                        return -1;
                    }
                    
                    if (available_row == available_row_2 && available_col == available_col_2) {
                        //seat still vacant
                        double_checked = 1;
                        //printf("matched\n");
                        reserve( i, available_row, available_col, id);
                        sem_post(&writer_sem);
                        gettimeofday((t->end_time)+id, NULL);
                        //writer end
                    }
                    
                    //writer end
                    sem_post(&writer_sem);
                }
            }
        }
    }

    // Don't forget the gettimeofday() call when you add another return statement
    return 0;
}

// Recommendation for first-fit:
// This function returns -1 if there are not enough consecutive seats
// Return the starting column id if there are enough seats

// You may declare other functions for best-fit and bonus:

int row_check(int row, int seats){
    int count = 0;
    int i;

    for(i=0; i<COL; i++){
        if(t->table[row][i] == 0){
            count++;
            if(count == seats)
                return (i-seats+1);
        }
        else
            count = 0;
    }

    return -1;
}
