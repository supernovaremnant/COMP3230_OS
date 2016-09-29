#include <stdlib.h>
#include <unistd.h>

int main(){

	char* a[512];
	int i;

while(1){
	for(i=0; i<512; i++){
		a[i] = malloc(512);
	}
	
	for(i=0; i<256; i++)
		a[i][0] = 'a';

	usleep(50000);

	for(i=0; i<512; i++)
		free(a[i]);

	usleep(50000);
}
	return 0;
}
