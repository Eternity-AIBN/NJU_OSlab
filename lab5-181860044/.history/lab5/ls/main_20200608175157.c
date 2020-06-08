#include "lib.h"
#include "types.h"

int main(void) {
	char ch[10];
	scanf("%10s",ch);
	if(ch==0){
		exit();
	}
	ls(ch); 
	return 0;
}
