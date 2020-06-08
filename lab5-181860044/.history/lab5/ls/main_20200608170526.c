#include "lib.h"
#include "types.h"

int main(void) {
	printf("Come into ls\n");
	char dest[10];
	scanf("%s",&dest);
	ls(dest); 
	exit();
	return 0;
}
