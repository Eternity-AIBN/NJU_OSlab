#include "types.h"
#include "lib.h"
int uEntry(void) {
	int fd = 0;
	int i = 0;
	char tmp = 0;

	/*ls("/"); 
	ls("/boot/"); 
	ls("/dev/");
	ls("/usr/"); */ 

	printf("create /usr/test and write alphabets to it\n");
	fd = 0;//open("/usr/test", O_READ | O_WRITE | O_CREATE); 
	printf("fd = %d\n",fd);
	for (i = 0; i < 512; i ++) { 
		tmp = (char)(i % 26 + 'A');
		write(fd, (uint8_t*)&tmp, 1);
	}
	close(fd);
	/*ls("/usr/"); 
	cat("/usr/test"); */
	exit();
	return 0;
}