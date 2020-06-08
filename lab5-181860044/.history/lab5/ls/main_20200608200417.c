#include "lib.h"
#include "types.h"

int main(void) {
	char ch[10];
	scanf("%10s",ch);
	/*if(strCmp(ch,"-1",2)==0){
		printf("exit\n");
		exit();
	} */
	ls(ch); 
	exit();
	return 0;
}
