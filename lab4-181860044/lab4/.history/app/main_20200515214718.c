/* #include "lib.h"
#include "types.h"

int uEntry(void) {
	char ch;
	printf("Input: 1 for bounded_buffer\n       2 for philosopher\n       3 for reader_writer\n");
	scanf("%c", &ch);
	switch (ch) {
		case '1':
			exec("/usr/bounded_buffer", 0);
			break;
		case '2':
			exec("/usr/philosopher", 0);
			break;
		case '3':
			exec("/usr/reader_writer", 0);
			break;
		default:
			break;
	}
	exit();
	return 0;
}*/
#include "lib.h"
#include "types.h"
int uEntry(void) {
	int i = 4;
	int ret = 0;
	int value = 2;
	sem_t sem;
	printf("Father Process: Semaphore Initializing.\n");
	ret = sem_init(&sem, value);
	if (ret == -1) {
		printf("Father Process: Semaphore Initializing Failed.\n");
		exit();
	}
	ret = fork();
	if (ret == 0) {
		while( i != 0) {
			i --;
			printf("Child Process: Semaphore Waiting.\n");
			sem_wait(&sem);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}
	else if (ret != -1) {
		while( i != 0) {
			i --;
			printf("Father Process: Sleeping.\n");
			sleep(128);
			printf("Father Process: Semaphore Posting.\n");
			sem_post(&sem);
		}
		printf("Father Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}

	return 0;
}