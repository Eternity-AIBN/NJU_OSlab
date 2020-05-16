#include "lib.h"
#include "types.h"
#define BUFFER_SIZE 8

int main(void) {
	// TODO in lab4
	printf("bounded_buffer\n");
	int ret = 0;
	int n = BUFFER_SIZE; //buffer size
	sem_t mutex, fullBuffers, emptyBuffers;
	ret = sem_init(&mutex, 1);
	if(ret == -1){
		printf("Init semaphone error!\n");  
		return -1;  
	}
	ret = sem_init(&fullBuffers, 0);
	if(ret == -1){
		printf("Init semaphone error!\n");  
		return -1;  
	}
	ret = sem_init(&emptyBuffers, n);
	if(ret == -1){
		printf("Init semaphone error!\n");  
		return -1;  
	}
	//printf("pid %d: hello world\n",getpid());

	for(int i = 0; i < 4; ++i){ //To get five process by fork, one consumer and four producers
		ret = fork();
		if(ret == -1){
			printf("Fork error!\n");  
			return -1;  
		}
		if(ret == 0){ //Child process
			break;
		}
	}

	printf("pid %d: hello world\n",getpid());

	sem_destroy(&mutex);
	sem_destroy(&fullBuffers);
	sem_destroy(&emptyBuffers);
	while(1);
	exit();
	return 0;
}
