#include "lib.h"
#include "types.h"

int main(void) {
	// TODO in lab4
	printf("bounded_buffer\n");
	int ret = 0;
	int n = 8; //buffer size
	sem_t mutex, fullBuffers, emptyBuffers;
	sem_init(&mutex, 1);
	sem_init(&fullBuffers, 0);
	sem_init(&emptyBuffers, n);

	sem_destroy(&mutex);
	sem_destroy(&fullBuffers);
	sem_destroy(&emptyBuffers);
	exit();
	return 0;
}
