#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PATH_MAX 204800
char filename[PATH_MAX];
char null_data[PATH_MAX];

int target_pid; //The pid of target application
int block_pid;	//The pid to be blocked from triggering the change of application

void null(){
	while(1){
		usleep(10000000);

		fprintf(stderr, "null\n");

		int i;
		for(i=0; i<50; i++){
			null_data[i*4096] = 2;
		}
	}
}

static int usage(char *me)
{
	fprintf(stderr, "%s: filename args\n", me);
	fprintf(stderr, "Run program, and print VmSize, VmData and VmStk (in KiB) to stdout\n");

	return 0;
}

static int child(int argc, char **argv)
{
	/* Pseudo-app null*/
	if(strcmp(argv[1], "null") == 0){
		null();
	}

	char **newargs = malloc(sizeof(char *) * argc);
	int i;
	
	/* We can't be certain that argv is NULL-terminated, so do that now */
	for (i = 0; i < argc - 1; i++)
	{
		newargs[i] = argv[i+1];
	}
	newargs[argc - 1] = NULL;
	
	/* Launch the child */
	execvp(argv[1], newargs);
	
	return 0;
}

static void sig_chld(int dummy)
{
    int status, child_val;
	int pid;
	
	(void) dummy;

    //wait for child process state to change ...
	pid = waitpid(-1, &status, WNOHANG);
    //pid is the child process who terminate
    if (pid < 0)
    {
        fprintf(stderr, "waitpid failed\n");
        return;
    }
	
	/* pid blocked */
    //signal comes from children
    if(pid == block_pid){
        fprintf(stderr, "signal coming from created terminal will be ignored. \n");
        return;
    }
		
	/* signal not from children */
	if (pid != target_pid ) 
	//Task 2: Use the new terminal to send SIGCHLD to parent. Fill in before "return;"
	//        1. Read and store the path to new program from the original terminal (assume no argument)
	//	  	  2. The current target will be terminated, so block its pid
	//        3. Fork and exec the new application
	//        4. Terminate original child
	{
        char new_program_to_run [PATH_MAX];
        scanf("Received SIGCHILD not from target application \n Please input the path to the new target application : %s ", &new_program_to_run );
        fprintf(stderr, "new program %s is going to be executed", new_program_to_run);
        
        //terminate old program
        int kill_status = kill(target_pid, SIGTERM);
        
        //run new program
        int new_created_status = execlp( &new_program_to_run, &new_program_to_run, NULL);
        
        if( new_created_status == -1 )
        {
            fprintf(stderr, "fail to create new program \n");
        }
        
        
		return;
	}
	

	/* Get child status value */
    if (WIFEXITED(status))
    {
        child_val = WEXITSTATUS(status);
        exit(child_val);
    }
}

static int main_loop(char *filename)
{
	char *line;
	char *vmsize;
	char *vmdata;
	char *vmstk;
	char *vmrss;
	
	size_t len;
	
	FILE *f;

	vmsize = NULL;
	vmdata = NULL;
	vmstk = NULL;
	vmrss = NULL;
	line = malloc(128);
	len = 128;
	
	f = fopen(filename, "r");
	if (!f) return 1;
	
	/* Read memory size data from /proc/pid/status */
	while (!vmsize || !vmdata || !vmstk || !vmrss)
	{
		if (getline(&line, &len, f) == -1)
		{
			/* Some of the information isn't there, die */
			return 1;
		}
		
		/* Find VmSize */
		if (!strncmp(line, "VmSize:", 7))
		{
			vmsize = strdup(&line[7]);
		}
		
		/* Find VmData */
		else if (!strncmp(line, "VmData:", 7))
		{
			vmdata = strdup(&line[7]);
		}
		/* Find VmStk */
		else if (!strncmp(line, "VmStk:", 6))
		{
			vmstk = strdup(&line[6]);
		}
		/* Find VmRSS */
		else if (!strncmp(line, "VmRSS:", 6))
		{
			vmrss = strdup(&line[6]);
		}
	}
	free(line);
	
	fclose(f);

	/* Get rid of " kB\n"*/
	len = strlen(vmsize);
	vmsize[len - 4] = 0;
	len = strlen(vmdata);
	vmdata[len - 4] = 0;
	len = strlen(vmstk);
	vmstk[len - 4] = 0;
	len = strlen(vmrss);
	vmrss[len - 4] = 0;
	
	/* Output results to stdout */
	printf("0:%s\n1:%s\n2:%s\n3:%s\n", vmsize, vmdata, vmstk, vmrss);
	
	free(vmsize);
	free(vmdata);
	free(vmstk);
	free(vmrss);
	
	/* Success */
	return 0;
}


int main(int argc, char **argv)
{
	struct sigaction act;
	
	if (argc < 2) return usage(argv[0]);

	//Task 1: Fork a new process and spawn a terminal. Output the pid of main process
	//        The terminal takes multiple steps of setup, so the final terminal is not the child process you create.
	//        USE block_pid for this fork()
    block_pid = fork();

    if( block_pid == 0)
    {
        //child process
        fprintf(stderr, "this is child process PID : %d \n ", getpid() );
        fprintf(stderr, "going to execute terminal process \n");
        execlp("/usr/bin/xfce4-terminal", "/usr/bin/xfce4-terminal", NULL);
        //process table over write here
        //anything added here won't be executed
        
    }else if ( block_pid < 0)
    {
        fprintf(stderr, "fail to create child process \n");
    }
    else{
        //main process
        fprintf(stderr, "this is the main process PID : %d \n", getpid() );
        fprintf(stderr, "New terminal spawned. Please send the signal to pid %d. ", getpid() );
    }
    
	/* null_data is modified */
	int i;
	for(i=0; i<50; i++){
		null_data[i*4096] = 1;
	}
	
    //create monitor child process
	target_pid = fork();
	
	if (!target_pid) return child(argc, argv); //run child process

    //main process continue ...
    
	//Set the signal handler function as sig_chld
	act.sa_handler = sig_chld;

    /* We don't want to block any other signals */
    sigemptyset(&act.sa_mask);
	
	act.sa_flags = SA_NOCLDSTOP;

	if (sigaction(SIGCHLD, &act, NULL) < 0)
    {
        fprintf(stderr, "sigaction failed\n");
        return 1;
    }
	
    //target pid is the monitor pid
	snprintf(filename, PATH_MAX, "/proc/%d/status", target_pid);
	
	/* Continual scan of proc */
	while (1)
	{
		main_loop(filename);
		/* Wait for 0.1 sec */
		usleep(100000);
		snprintf(filename, PATH_MAX, "/proc/%d/status", target_pid);
	}
	
	return 1;
}
