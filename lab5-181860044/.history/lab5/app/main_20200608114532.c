#include "types.h"
#include "lib.h"
/*int uEntry(void) {
	int fd = 0;
	int i = 0;
	char tmp = 0;

	ls("/"); 
	ls("/boot/"); 
	ls("/dev/");
	ls("/usr/"); 

	printf("create /usr/test and write alphabets to it\n");
	fd = open("/usr/test", O_READ | O_WRITE | O_CREATE); 
	for (i = 0; i < 26; i ++) { 
		tmp = (char)(i % 26 + 'A');
		write(fd, (uint8_t*)&tmp, 1);
	}
	close(fd);
	ls("/usr/"); 
	cat("/usr/test"); 
	exit();
	return 0;
} */

int uEntry(void) {
	//char ch[10];
	//char destPathName[100];
	printf("Input: ls <destPathName>\n       cat <destPathName>\n       3 for reader_writer\n");
	//scanf("%s %s", &ch, &destPathName);
	//if(strCmp(ch,"ls",2)==0){
	ls("/usr");
	exec("/usr/ls", 0);
	//}
	exit();
	return 0;
}