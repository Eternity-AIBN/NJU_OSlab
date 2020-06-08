#include "lib.h"
#include "types.h"

int main(void) {
	char ch[100];
	scanf("%100s",ch);
	int fd = open(ch, O_READ | O_WRITE | O_CREATE); 
	printf("open %s\n",ch);
	printf("write: ");
	for (int i = 0; i < 26; i ++) { 
		char tmp = (char)(i % 26 + 'A');
		write(fd, (uint8_t*)&tmp, 1);
		printf("%c",tmp);
	}
	printf("\n");
	close(fd);
	exit();
	return 0;
}
