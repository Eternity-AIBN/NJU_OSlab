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
	int data = 2020;
	int data1 = 1000;
	int i = 4;
	int ret = fork();
	if (ret == 0) {
		while (i != 0) {
			i--;
			printf("Child Process: %d, %d\n", data, data1);
			write(SH_MEM, (uint8_t *)&data, 4, 0); // define SH_MEM 3
			printf("%d\n",(uint32_t)&data);
			data += data1;
			sleep(128);
		}
	exit();
	} else if (ret != -1) {
		while (i != 0) {
			i--;
			read(SH_MEM, (uint8_t *)&data1, 4, 0);
			printf("Father Process: %d, %d\n", data, data1);
			printf("%d\n",(uint32_t)&data);
			sleep(128);
		}
		exit();
	}
	return 0;
}