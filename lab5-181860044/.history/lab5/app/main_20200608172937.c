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

int strCmp (const char *srcString, const char *destString, int size) { // compre first 'size' bytes
    int i = 0;
    while (i != size) {
        if (srcString[i] != destString[i])
            return -1;
        else if (srcString[i] == 0)
            return 0;
        else
            i ++;
    }
    return 0;
}

int uEntry(void) {
	char ch[10];
	//char destPathName[100];
	printf("Input: ls <destPathName>\n       cat <destPathName>\n       3 for reader_writer\n");
	scanf("%3s", ch);
	printf("%s\n",ch);
	if(strCmp(ch,"ls",2)==0){
		exec("/usr/ls", 0);
	//scanf("%s",&ch);
	}
	exit();
	return 0;
}