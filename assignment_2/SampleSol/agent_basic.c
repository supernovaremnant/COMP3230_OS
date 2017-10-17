#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "trs.h"
#include <sys/time.h>

extern int req[AGENT_NO][COL*ROW];
extern int served_req[AGENT_NO][COL*ROW][3];		// served requests
extern rt *t;							// reservation table
extern sem_t reader_sem;				// semaphore for reading
extern sem_t writer_sem;				// semaphore for writing
extern int mode;
extern void reserve(int req_id, int row, int col, int agent_id);
extern void withdraw(int req_id, int agent_id);

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







// you have to insert your code inside this file
// It may call other functions written by you, if there is any.
// if "mode" is 0, agent() should use the first-fit algorithm
// if "mode" is 1, agent() should use the best-fit algorithm

void *agent(void *data)  {
    
    int id = *((int *) data);
    
    gettimeofday((t->start_time)+id, NULL);
    
    int i;
    int request;
    
    for(i=0; i<COL*ROW; i++){
        request = req[id][i];
        if(request <= 0){
	    	// Withdraw request
	    	// This part does not differ in Mode 0 and Mode 1
			sem_wait(&writer_sem);
			withdraw(i, id);
			sem_post(&writer_sem);
        }
        else{
            // Reservation request
            // Need to first read the reservation table to look for potential seats
            // Double-check when you get the writing permission
            // If the seats are gone, keep the double_checked flag as 0
            // This request will be processed again;
            // But if none of the rows can satisfy this request, the thread should terminate immediately
            
                int double_checked = 0;
                int iRow=0, iCol=-1;
				int s, seat_count, min, best_row;

                while( !double_checked ){
                    // Read table (need to take different actions for Mode 0 and Mode 1)
                    // You may declare functions as you like
                        
                    sem_wait (&reader_sem); //p(&reader_sem)
                    t->reader_count = t->reader_count+1;
                    
                    
                    if (t->reader_count == 1)
                    {
                        sem_wait (&writer_sem); //p(&writer_sem);
                    }
                
                    sem_post(&reader_sem);

                    if(mode == 0){
                    	for(iRow=0; iRow<ROW; iRow++){
                        	iCol  = row_check(iRow,request);
                        	if(iCol != -1)
                            	break;
                    	}
					}
					else{
						iCol = -1;
						min = COL+1;
                        best_row = -1;

						for (iRow=0; iRow<ROW; iRow++){
							seat_count = 0;

							for(s=0; s<COL; s++){
								if(t->table[iRow][s] == 0){
									seat_count++;
								}	
								else {//non-zero
									if(seat_count == request){//perfect fit
										min = seat_count;
										iCol = s - seat_count;
										break;
									}
									else if(seat_count > request && seat_count < min){
										min = seat_count;
        								iCol = s - seat_count;
										best_row = iRow;
									}
								
									seat_count = 0;
								}
							}

                                if(seat_count == request){//perfect fit
										min = seat_count;
										iCol = s - seat_count;
										break;
								}
								else if(seat_count > request && seat_count < min){
										min = seat_count;
        								iCol = s - seat_count;
										best_row = iRow;
								}

							if(min == request)
								break;
						}

						if(min != request)
							iRow = best_row;
					}
                    
                    sem_wait(&reader_sem);
                    
                    t->reader_count = t->reader_count-1;
                    
                    if (t->reader_count == 0){
                        sem_post(&writer_sem);
                    }
                    
                    sem_post(&reader_sem);
                
                    // Write table (need to confirm the seats are not taken)
                    
                    if(iCol == -1){
                        gettimeofday((t->end_time)+id, NULL);
                        return 0;
                    }
                    
                    sem_wait(&writer_sem);
                    
                    double_checked = 1;
                    int col;
                    for(col =iCol; col<iCol+request; col++){
                        if(t->table[iRow][col] != 0)
                        {
                            double_checked = 0;
                            break;
                        }
                    }
                        
                    
                    if (double_checked){
                        reserve(i, iRow , iCol, id); 
                        sem_post(&writer_sem);
                    }
                    else {
                        sem_post(&writer_sem);
                    }
                    
				}

	
                
            
            }
            
            
            
        
        }
        
    gettimeofday((t->end_time)+id, NULL);
    return 0;
    
}

