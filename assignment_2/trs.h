#define ROW 128			// number of rows
#define COL 128			// number of columns
#define MIN_SEAT_REQ 1		// min. no. of seats to be reserved in a request
#define MAX_SEAT_REQ 20		// max. no. of seats to be reserved in a request
#define WITHDRAW 3		// one withdrawal in every WITHDRAW reqs
#define AGENT_NO 3		// mo. of agent threads

typedef struct Rt_t {
	int reader_count;	// used only in basic programming part
	int reader_counts[ROW]; // used only in bonus
	int table[ROW][COL];	// reservation table
	int seats[AGENT_NO];	// used only in trs.c, you may ignore
	struct timeval start_time[AGENT_NO];	//Starting time of each thread
	struct timeval end_time[AGENT_NO];		//Ending time of each thread
} rt;

