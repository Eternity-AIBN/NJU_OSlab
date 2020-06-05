#include <stdio.h>
#include "utils.h"
#include "data.h"
#include "func.h"

int main(int argc, char *argv[]) {
    char driver[NAME_LENGTH];
    char srcFilePath[NAME_LENGTH];
    char destFilePath[NAME_LENGTH];
    for(int i=0;i<4;++i)
        printf("%s\n",argv[i]);

    stringCpy("fs.bin", driver, NAME_LENGTH - 1);
    format(driver, SECTOR_NUM, SECTORS_PER_BLOCK);

    stringCpy("/boot", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);

    stringCpy(argv[1], srcFilePath, NAME_LENGTH - 1);
    stringCpy("/boot/initrd", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);

    stringCpy(argv[3], srcFilePath, NAME_LENGTH - 1);
    printf("%s\n",argv[3]);
    stringCpy("/usr/bounded_buffer", srcFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath); 

    //rm '/usr/bounded_buffer'
    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);   //Before
    stringCpy("/usr/bounded_buffer", destFilePath, NAME_LENGTH - 1);
    rm(driver, destFilePath);   //After
    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);  

    /*stringCpy("/boot", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/boot/initrd", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath); */

    //rmdir '/usr':
    stringCpy("/", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);   //Before
    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    rmdir(driver, destFilePath);
    stringCpy("/", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);   //After
    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    rmdir(driver, destFilePath);
    stringCpy("/", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);   //After

    return 0;
}
