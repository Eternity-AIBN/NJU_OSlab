#include "lib.h"
#include "types.h"
#define N 5 

int main(void) {
	// TODO in lab4
	printf("philosopher\n");
	int ret = 0;
	sem_t Fork[5];
	for(int i=0; i<5; ++i){
		ret = sem_init(&Fork[i], 1);
		if(ret == -1){
			printf("Init semaphone error!\n");  
			return -1;  
		}
	}

	for(int i = 0; i < 4; ++i){ //To get five process by fork (pid = 1~5)
		ret = fork();
		if(ret == -1){
			printf("Fork error!\n");  
			return -1;  
		}
		if(ret == 0){ //Child process
			break;
		}
	}

	int id = 0;
	while(1){
		id = getpid();
		printf("Philosopher %d: think\n", id);
		sleep(128);  //Between think and eat
		if(id%2 == 0){
			sem_wait(&Fork[id]);
			sem_wait(&Fork[(id+1)%N]); 
		} 
		else {
			sem_wait(&Fork[(id+1)%N]); 
			sem_wait(&Fork[id]);
		}
		printf("Philosopher %d: eat\n", id);
		sleep(128);  //Between P-V operation
		sem_post(&Fork[id]); 
		sem_post(&Fork[(id+1)%N]); 
	}

	exit();
	return 0;
}
