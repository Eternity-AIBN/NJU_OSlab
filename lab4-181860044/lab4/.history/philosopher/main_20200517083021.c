#include "lib.h"
#include "types.h"
#define N 5 

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
	printf("num = %d\n",num);
	return num;
}

int main(void) {
	// TODO in lab4
	printf("philosopher\n");
	uint32_t seed = 2;  //For generator the random number
	write(SH_MEM, (uint8_t *)&seed, 4, 4);

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
		sleep(random());  //Between think and eat
		if(id%2 == 0){
			sem_wait(&Fork[id]);
			sem_wait(&Fork[(id+1)%N]); 
		} 
		else {
			sem_wait(&Fork[(id+1)%N]); 
			sem_wait(&Fork[id]);
		}
		printf("Philosopher %d: eat\n", id);
		sleep(random());  //Between P-V operation
		sem_post(&Fork[id]); 
		sem_post(&Fork[(id+1)%N]); 
	}

	exit();
	return 0;
}
