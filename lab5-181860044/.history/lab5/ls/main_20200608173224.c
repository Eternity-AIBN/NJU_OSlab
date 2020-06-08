#include "lib.h"
#include "types.h"

extern char ch[10];
int main(void) {
	printf("ch=%s\n",ch);
	ls("/"); 
	exit();
	return 0;
}
