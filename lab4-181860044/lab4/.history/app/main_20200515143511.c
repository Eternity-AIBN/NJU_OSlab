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
 	int dec = 0;
 	int hex = 0;
 	char str[6];
 	char cha = 0;
 	int ret = 0;
 	while(1){
 		printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
 		ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
 		printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
 		if (ret == 4)
 			break;
 	}
 	return 0;
}