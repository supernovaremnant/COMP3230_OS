#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "trs.h"

#define DEBUG 0

extern int req[AGENT_NO][COL*ROW];
extern int served_req[AGENT_NO][COL*ROW][3];		// served requests
extern rt *t;							// reservation table
extern sem_t reader_sem;				// semaphore for reading
extern sem_t writer_sem;				// semaphore for writing
extern int mode;
extern sem_t reader_sem_arr[ROW];
extern sem_t writer_sem_arr[ROW];

// you have to insert your code inside this file
// It may call other functions written by you, if there is any.
// if "mode" is 0, agent() should use the first-fit algorithm
// if "mode" is 1, agent() should use the best-fit algorithm 

void best_fit( int seats, int * row, int * col );
void first_fit( int seats, int * row, int * col);
void advance_fit(int seats, int * row, int * col );

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
            if (mode == 2)
            {
                int row_withdraw = served_req[id][req[id][i]*-1][0];
                int tmp = 0;
                int ret = sem_getvalue(&writer_sem_arr[row_withdraw], &tmp);
                if(tmp <= 0 && ret == 0)
                {
                    printf("writer busy sleep\n");
                    usleep(1);
                    //ret = sem_getvalue(&writer_sem_arr[row_withdraw], &tmp);    
                }else{
                    printf("got it\n");    
                }
                
                sem_wait(&writer_sem_arr[row_withdraw]);
                withdraw(i,id);
                sem_post(&writer_sem_arr[row_withdraw]);
            }else{
                //writer begin
                sem_wait( &writer_sem ); //lock writer
                withdraw(i, id);
                sem_post(&writer_sem);//unlock writer
                //writer end    
            }

            if (DEBUG)
            {
                printf("wit agent_id:%d, req_id:%d \n", id, i);
            }

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
                
                int available_row = -1; int available_col = -1; int row_i = 0;    

                if (mode == 2)
                {
                    advance_fit(request, &available_row, &available_col);

                }else{
                    //reader begin
                    sem_wait( &reader_sem ); 
                    t->reader_count += 1;
                    if (t->reader_count == 1) {
                        sem_wait(&writer_sem);
                    }
                    sem_post(&reader_sem);
                    
                    if (mode == 0)
                    {
                        first_fit(request, &available_row, &available_col);
                    }else{
                        best_fit(request, &available_row, &available_col);
                    }
                    
                    sem_wait(&reader_sem);
                    t->reader_count -= 1 ;
                    
                    if (t->reader_count == 0) {
                        sem_post(&writer_sem);
                    }
                    sem_post(&reader_sem);
                    //end of reader
                }

                //using data
                if( available_col == -1 ){
                    if(DEBUG)
                        printf("cannot find more seats, return\n");
                    sem_post(&writer_sem_arr[available_row]);
                    gettimeofday((t->end_time)+id, NULL);
                    return -1;
                }else{
                    if (DEBUG)
                    {
                        printf("req row:%d, col:%d, req_num:%d, agent_id:%d, req_id:%d \n", available_row, available_col, request, id, i);
                    }
                    
                    int available_row_2 = -1; int available_col_2 = -1; row_i = 0;
                        
                    if (mode == 2)
                    {
                        //writer begin
                        sem_wait(&writer_sem_arr[available_row]);

                        //double check
                        best_fit(request, &available_row_2, &available_col_2);
                        //advance_fit(request, &available_row_2, &available_col_2);

                        //use reader data
                        if( available_col_2 == -1 ){
                            if(DEBUG)
                                printf("cannot find more seats, end\n"); 
                            sem_post(&writer_sem_arr[available_row]);
                            gettimeofday((t->end_time)+id, NULL);
                            return -1;
                        }

                        //double checking 
                        if (available_row == available_row_2 && available_col == available_col_2) {
                            //seat still vacant 
                            double_checked = 1;
                            if (DEBUG)
                            {
                                printf("wr row:%d, col:%d, req_num:%d, agent_id:%d, req_id:%d \n\n", available_row, available_col, request, id, i);
                                
                            }
                            reserve( i, available_row, available_col, id);
                            //print_table(t->table);
                            //writer end
                        }
                        sem_post(&writer_sem_arr[available_row]);

                    }else{
                        //writer begin
                        sem_wait(&writer_sem);
                        
                        //double check
                        
                        if (mode == 0)
                        {
                            first_fit(request, &available_row_2, &available_col_2);
                        }else{
                            best_fit(request, &available_row_2, &available_col_2);
                        }
                        
                        //use reader data
                        if( available_col_2 == -1 ){
                            if(DEBUG)
                                printf("cannot find more seats, end\n"); 
                            sem_post(&writer_sem);
                            gettimeofday((t->end_time)+id, NULL);
                            return -1;
                        }
                        
                        //double checking
                        if (available_row == available_row_2 && available_col == available_col_2) {
                            //seat still vacant 
                            double_checked = 1;
                            if (DEBUG)
                            {
                                printf("wr row:%d, col:%d, req_num:%d, agent_id:%d, req_id:%d \n\n", available_row, available_col, request, id, i);
                            }
                            reserve( i, available_row, available_col, id);
                            if (DEBUG)
                            {
                                print_table(t->table);  
                            }
                            //writer end
                        }
                        sem_post(&writer_sem);    
                    }
                    
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

void best_fit( int seats, int * row, int * col ){
    int best_row = -1; 
    int best_col = -1; 
    int min_dis = COL+1;
    int dis;
    int count = 0;
    int i = 0;

    if (seats %2 == 0)
    {
        for (i = 0; i < ROW; ++i)
        {
            for (int j = 0; j < COL; ++j)
            {
                if (t->table[i][j] == 0)
                {
                    count ++;
                }else{
                    dis = count - seats;
                    if (dis >= 0 && dis < min_dis)
                    {
                        min_dis = dis; 
                        best_row = i;
                        best_col = j - count;
                    }
                    count = 0;
                }
            }
            dis = count - seats;
            if (dis >= 0 && dis < min_dis )
            {
                min_dis = dis;
                best_row = i; 
                best_col = COL - min_dis - seats;
            }
            count = 0;
        }
    }else{
        for (i = ROW - 1; i >= 0; --i)
        {
            for (int j = 0; j < COL; ++j)
            {
                if (t->table[i][j] == 0)
                {
                    count ++;
                }else{
                    dis = count - seats;
                    if (dis >= 0 && dis < min_dis)
                    {
                        min_dis = dis; 
                        best_row = i;
                        best_col = j - count;
                    }
                    count = 0;
                }
            }
            dis = count - seats;
            if (dis >= 0 && dis < min_dis )
            {
                min_dis = dis;
                best_row = i; 
                best_col = COL - min_dis - seats;
            }
            count = 0;
        }
    }
    
    if (best_row == -1 )
    {
        *row = -1; *col = -1;
    }else{
        *row = best_row; *col = best_col;
    }
}

void first_fit( int seats, int * row, int * col){
    int row_i = 0;   
    do{
        *col = row_check(row_i, seats);
        if( *col != -1 ){
            *row = row_i;
            break;
        }
        row_i ++;
    }while( row_i < ROW );
}

void advance_fit( int seats, int * row, int * col ){
    int best_row = -1; 
    int best_col = -1; 
    int min_dis = COL+1;
    int dis;
    int count = 0;
    int i = 0;

    if (seats %2 == 0)
    {
        for (i = 0; i < ROW; ++i)
        {   
            //unlock sem 
            sem_wait(&reader_sem_arr[i]);
            t->reader_counts[i] += 1;
            if ( t->reader_counts[i] == 1)
            {
                sem_wait(&writer_sem_arr[i]);
            }
            sem_post(&reader_sem_arr[i]);

            for (int j = 0; j < COL; ++j)
            {
                if (t->table[i][j] == 0)
                {
                    count ++;
                }else{
                    dis = count - seats;
                    if (dis >= 0 && dis < min_dis)
                    {
                        min_dis = dis; 
                        best_row = i;
                        best_col = j - count;
                    }
                    count = 0;
                }
            }
            dis = count - seats;
            if (dis >= 0 && dis < min_dis )
            {
                min_dis = dis;
                best_row = i; 
                best_col = COL - min_dis - seats;
            }
            count = 0;

            sem_wait(&reader_sem_arr[i]);
            t->reader_counts[i]--;
            if (t->reader_counts[i] == 0)
            {
                sem_post(&writer_sem_arr[i]);
            }
            sem_post(&reader_sem_arr[i]);
        }
    }else{
        for (i = ROW - 1; i >= 0 ; --i)
        {   
            //unlock sem 
            sem_wait(&reader_sem_arr[i]);
            t->reader_counts[i] += 1;
            if ( t->reader_counts[i] == 1)
            {
                sem_wait(&writer_sem_arr[i]);
            }
            sem_post(&reader_sem_arr[i]);

            for (int j = 0; j < COL; ++j)
            {
                if (t->table[i][j] == 0)
                {
                    count ++;
                }else{
                    dis = count - seats;
                    if (dis >= 0 && dis < min_dis)
                    {
                        min_dis = dis; 
                        best_row = i;
                        best_col = j - count;
                    }
                    count = 0;
                }
            }
            dis = count - seats;
            if (dis >= 0 && dis < min_dis )
            {
                min_dis = dis;
                best_row = i; 
                best_col = COL - min_dis - seats;
            }
            count = 0;

            sem_wait(&reader_sem_arr[i]);
            t->reader_counts[i]--;
            if (t->reader_counts[i] == 0)
            {
                sem_post(&writer_sem_arr[i]);
            }
            sem_post(&reader_sem_arr[i]);
        }
    }
    
    if (best_row == -1 )
    {
        *row = -1; *col = -1;
    }else{
        *row = best_row; *col = best_col;
    }
}