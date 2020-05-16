#include "lib.h"
#include "types.h"

int main(void) {
	// TODO in lab4
	printf("reader_writer\n");
	int ret = 0;
	int Rcount = 0;
	write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
	sem_t WriteMutex, CountMutex;
	ret = sem_init(&WriteMutex, 1);
	if(ret == -1){
		printf("Init semaphone error!\n");  
		return -1;  
	}
	ret = sem_init(&CountMutex, 1);
	if(ret == -1){
		printf("Init semaphone error!\n");  
		return -1;  
	}

	for(int i = 0; i < 5; ++i){ //To get six process by fork, three reader(pid = 1~3) and three writer(pid = 4~6)
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
	if(getpid() >= 1 && getpid() <= 3){  //reader process
		while(1){
			sem_wait(&CountMutex);
			sleep(128);
			read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
			if(Rcount == 0){
				id = getpid();
				sem_wait(&WriteMutex);
			}
			Rcount++;
			write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
			sem_post(&CountMutex);
			id = getpid();
			printf("Reader %d: read, total %d reader\n", id, Rcount);
			sleep(128);
			sem_wait(&CountMutex);
			sleep(128);
			read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
			Rcount--;
			write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
			if(Rcount == 0){
				sem_post(&WriteMutex);
			}
			sem_post(&CountMutex);
			printf("Reader %d read over, total %d reader\n", id, Rcount);
		}
	}
	else if(getpid() >= 4 && getpid() <= 6){  //writer process
		while(1){
			sem_wait(&WriteMutex);
			id = getpid();		
			printf("Writer %d: write\n", id);
			sleep(128);
			sem_post(&WriteMutex);
			printf("Write over\n");
		}
	}

	exit();
	return 0;
}
