#include <stdlib.h>
#include <sys/types.h>

int main (int argc, char ** argv )
{
	int c = 5;
	int child = fork();
	
	if(child != 0)
	{
		child=fork();
		c+=10;
		if( child )
			c+= 5;

	}
	printf("c: %d with pid %d \n", c, getpid());
	return 0;
}
