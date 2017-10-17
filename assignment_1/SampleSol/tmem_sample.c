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

	pid = waitpid(-1, &status, WNOHANG);
    if (pid < 0)
    {
        fprintf(stderr, "waitpid failed\n");
        return;
    }
	
	/* pid blocked */
	if(pid == block_pid)
		return;
	
	/* signal not from children */
	if (pid != target_pid ) /*return;*/
	//Task 2: Use the new terminal to send SIGCHLD to parent. Replace "return;" with a clause to handle SIGCHLD from terminal
	//        1. Read and store the path to new program from the original terminal (assume no argument)
	//	  	  2. The current target will be terminated, so block its pid
	//        3. Fork and exec the new application
	//        4. Terminate original child
	{
		fprintf(stderr, "Received SIGCHLD not from target application\n");
		fprintf(stderr, "Please input the path to the new target application\n");
		char newpath[128];
		scanf("%s", newpath);

		pid_t new_pid = fork();
		if (new_pid < 0)
    	{
        	fprintf(stderr, "fork failed\n");
        	exit(1);
    	}
		else if(new_pid == 0)
			execlp(newpath, newpath, NULL);
		
		block_pid = target_pid;
		target_pid = new_pid;

		kill(block_pid, SIGTERM);

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

	//Task 1: Fork a new process and spawn a terminal. Output the 
	//        The terminal takes multiple steps of setup, so the final terminal is not the child process you create.
	//        USE block_pid for this fork()
	block_pid = fork();

	if (block_pid < 0)
    {
        fprintf(stderr, "fork failed\n");
        return 1;
    }
	else if(block_pid == 0)
		execlp("/usr/bin/xfce4-terminal", "xfce4-terminal", NULL);

	fprintf(stderr, "New terminal spawned. Please send the signal to pid %d\n", getpid());

	/* null_data is modified */
	int i;
	for(i=0; i<50; i++){
		null_data[i*4096] = 1;
	}
	
	target_pid = fork();
	
	if (!target_pid) return child(argc, argv);

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
