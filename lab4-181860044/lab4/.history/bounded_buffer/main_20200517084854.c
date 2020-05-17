#include "lib.h"
#include "types.h"
#define BUFFER_SIZE 5

uint32_t random(){
	uint32_t num, lfsr, num_4, num_3, num_2, num_0;
	read(SH_MEM, (uint8_t *)&num, 4, 4);

	num_4 = (num >> 3) % 2;
	num_3 = (num >> 2) % 2;
	num_2 = (num >> 1) % 2;
	num_0 = num % 2;
	lfsr = num_4^num_3^num_2^num_0;

	num = (lfsr << 7) + ((num>>1) & 0x7f);
	if (num == 0)
		num = 1;
	write(SH_MEM, (uint8_t *)&num, 4, 4);
	return num;
}

int main(void) {
	// TODO in lab4
	printf("bounded_buffer\n");
	uint32_t seed = 2;  //For generator the random number
	write(SH_MEM, (uint8_t *)&seed, 4, 4);

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

	for(int i = 0; i < 4; ++i){ //To get five process by fork, one consumer(pid = 1) and four producers(pid = 2~5)
		ret = fork();
		if(ret == -1){
			printf("Fork error!\n");  
			return -1;  
		}
		if(ret == 0){ //Child process
			break;
		}
	}
	//printf("pid %d: hello world\n",getpid());
	if(getpid() == 1){  //consumer process
		while(1){
			sem_wait(&fullBuffers);
			sem_wait(&mutex);
			printf("Consumer : consume\n");
			sleep(random());
			sem_post(&mutex);
			sem_post(&emptyBuffers);
			//sleep(random());
		}
		
	}
	else if(getpid()>1 && getpid()<6){ //producer process
		while(1){
			sleep(random()+200);
			sem_wait(&emptyBuffers);
			sem_wait(&mutex);
			int id = getpid() - 1;  //producer id, 1~4
			printf("Producer %d: produce\n", id);
			sleep(random());
			sem_post(&mutex);
			sem_post(&fullBuffers);
		}
		
	}

	sem_destroy(&mutex);
	sem_destroy(&fullBuffers);
	sem_destroy(&emptyBuffers);
	exit();
	return 0;
}
