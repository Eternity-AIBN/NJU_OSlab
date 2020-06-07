#include "x86.h"
#include "device.h"
#include "fs.h"

int readSuperBlock(SuperBlock *superBlock) {
    diskRead((void*)superBlock, sizeof(SuperBlock), 1, 0);
    return 0;
}

int readBlock (SuperBlock *superBlock, Inode *inode, int blockIndex, uint8_t *buffer) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];
    
    if (blockIndex < bound0) {
        diskRead((void*)buffer, sizeof(uint8_t), superBlock->blockSize, inode->pointer[blockIndex] * SECTOR_SIZE);
        return 0;
    }
    else if (blockIndex < bound1) {
        diskRead((void*)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
        diskRead((void*)buffer, sizeof(uint8_t), superBlock->blockSize, singlyPointerBuffer[blockIndex - bound0] * SECTOR_SIZE);
        return 0;
    }
    else {
        return -1;
    }
}

int readInode(SuperBlock *superBlock, Inode *destInode, int *inodeOffset, const char *destFilePath) {
    int i = 0;
    int j = 0;
    int ret = 0;
    int cond = 0;
    *inodeOffset = 0;
    uint8_t buffer[superBlock->blockSize];
    DirEntry *dirEntry = NULL;
    int count = 0;
    int size = 0;
    int blockCount = 0;

    if (destFilePath == NULL || destFilePath[count] == 0) {
        return -1;
    }
    ret = stringChr(destFilePath, '/', &size);
    if (ret == -1 || size != 0) {
        return -1;
    }
    count += 1;
    *inodeOffset = superBlock->inodeTable * SECTOR_SIZE;
    //putString("\nsssss\n");
    diskRead((void *)destInode, sizeof(Inode), 1, *inodeOffset);

    while (destFilePath[count] != 0) {
        ret = stringChr(destFilePath + count, '/', &size);
        if(ret == 0 && size == 0) {
            return -1;
        }
        if (ret == -1) {
            cond = 1;
        }
        else if (destInode->type == REGULAR_TYPE) {
            return -1;
        }
        blockCount = destInode->blockCount;
        for (i = 0; i < blockCount; i ++) {
            ret = readBlock(superBlock, destInode, i, buffer);
            if (ret == -1) {
                return -1;
            }
            dirEntry = (DirEntry *)buffer;
            for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j ++) {
                if (dirEntry[j].inode == 0) {
                    continue;
                }
                else if (stringCmp(dirEntry[j].name, destFilePath + count, size) == 0) {
                    *inodeOffset = superBlock->inodeTable * SECTOR_SIZE + (dirEntry[j].inode - 1) * sizeof(Inode);
                    diskRead((void *)destInode, sizeof(Inode), 1, *inodeOffset);
                    break;
                }
            }
            if (j < superBlock->blockSize / sizeof(DirEntry)) {
                break;
            }
        }
        if (i < blockCount) {
            if (cond == 0) {
                count += (size + 1);
            }
            else {
                return 0;
            }
        }
        else {
            return -1;
        }
    }
    return 0;
}

//Add
int getAvailInode (SuperBlock *superBlock, int *inodeOffset) {
    int j = 0;
    int k = 0;
    int inodeBitmapOffset = 0;
    int inodeTableOffset = 0;
    InodeBitmap inodeBitmap;

    if (superBlock->availInodeNum == 0)
        return -1;
    superBlock->availInodeNum--;

    inodeBitmapOffset = superBlock->inodeBitmap;
    inodeTableOffset = superBlock->inodeTable;
    diskRead((void*)&inodeBitmap, sizeof(InodeBitmap), 1, inodeBitmapOffset * SECTOR_SIZE);
    for (j = 0; j < superBlock->availInodeNum / 8; j++) {
        if (inodeBitmap.byte[j] != 0xff) {
            break;
        }
    }
    for (k = 0; k < 8; k++) {
        if ((inodeBitmap.byte[j] >> (7-k)) % 2 == 0) {
            break;
        }
    }
    inodeBitmap.byte[j] = inodeBitmap.byte[j] | (1 << (7 - k));

    *inodeOffset = inodeTableOffset * SECTOR_SIZE + (j * 8 + k) * sizeof(Inode);

    diskWrite((void*)superBlock, sizeof(SuperBlock), 1, 0);
    diskWrite((void*)&inodeBitmap, sizeof(InodeBitmap), 1, inodeBitmapOffset * SECTOR_SIZE);

    return 0;
}

int allocInode (SuperBlock *superBlock,Inode *fatherInode,int fatherInodeOffset,
    Inode *destInode,int *destInodeOffset,const char *destFilename,int destFiletype){
    int i = 0;
    int j = 0;
    int ret = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];
    int length = stringLen(destFilename);

    if (destFilename == NULL || destFilename[0] == 0)
        return -1;

    if (superBlock->availInodeNum == 0)
        return -1;
    
    for (i = 0; i < fatherInode->blockCount; i++) {
        ret = readBlock(superBlock, fatherInode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j++) {
            if (dirEntry[j].inode == 0) // a valid empty dirEntry
                break;
            else if (stringCmp(dirEntry[j].name, destFilename, length) == 0)
                return -1; // file with filename = destFilename exist
        }
        if (j < superBlock->blockSize / sizeof(DirEntry))
            break;
    }
    if (i == fatherInode->blockCount) {
        ret = allocBlock(superBlock, fatherInode, fatherInodeOffset);
        if (ret == -1)
            return -1;
        fatherInode->size = fatherInode->blockCount * superBlock->blockSize;
        setBuffer(buffer, superBlock->blockSize, 0);
        dirEntry = (DirEntry *)buffer;
        j = 0;
    }
    // dirEntry[j] is the valid empty dirEntry, it is in the i-th block of fatherInode.
    ret = getAvailInode(superBlock, destInodeOffset);
    if (ret == -1)
        return -1;

    stringCpy(destFilename, dirEntry[j].name, NAME_LENGTH);
    dirEntry[j].inode = (*destInodeOffset - superBlock->inodeTable * SECTOR_SIZE) / sizeof(Inode) + 1;
    ret = writeBlock(superBlock, fatherInode, i, buffer);
    if (ret == -1)
        return -1;
    
    diskWrite((void*)fatherInode, sizeof(Inode), 1, fatherInodeOffset);

    destInode->type = destFiletype;
    destInode->linkCount = 1;
    destInode->blockCount = 0;
    destInode->size = 0;

	diskWrite((void*)destInode, sizeof(Inode), 1, *destInodeOffset);

    return 0;
}

int removeAllocedBlock (SuperBlock *superBlock, int *blockOffset) {
    int j = 0;
    int k = 0;
    int blockBitmapOffset = 0;
    BlockBitmap blockBitmap;

    superBlock->availBlockNum++;
    
    blockBitmapOffset = superBlock->blockBitmap;
    diskRead((void *)&blockBitmap, sizeof(BlockBitmap), 1, blockBitmapOffset * SECTOR_SIZE);

    //printf("blockOffset = %d\n",*blockOffset);
    uint32_t temp = (*blockOffset-superBlock->blocks)/(superBlock->blockSize/SECTOR_SIZE);
    j = temp/8;
    k = temp%8;

    //printf("j = %d, k = %d\n",j,k);
    //printf("blockBitmap.byte[j] = %d\n",blockBitmap.byte[j]);
    uint8_t tmp = ~(1 << (7-k));
    blockBitmap.byte[j] = blockBitmap.byte[j] & tmp;
    //printf("blockBitmap.byte[j] = %d\n",blockBitmap.byte[j]);
    //printf("cccc\n");
    //*blockOffset = 0;
    diskWrite((void *)superBlock, sizeof(SuperBlock), 1, 0);
    diskWrite((void *)&blockBitmap, sizeof(BlockBitmap), 1, blockBitmapOffset * SECTOR_SIZE);
    return 0;
}

int removeAllocedInode (SuperBlock *superBlock, int *inodeOffset) {
    int j = 0;
    int k = 0;
    int inodeBitmapOffset = 0;
    int inodeTableOffset = superBlock->inodeTable;;
    InodeBitmap inodeBitmap;

    superBlock->availInodeNum++;

    inodeBitmapOffset = superBlock->inodeBitmap;
    diskRead((void *)&inodeBitmap, sizeof(BlockBitmap), 1, inodeBitmapOffset * SECTOR_SIZE);
    
    uint32_t temp = (*inodeOffset - inodeTableOffset * SECTOR_SIZE) / sizeof(Inode);
    j = temp/8;
	k = temp%8;
    uint8_t tmp = ~(1 << (7-k));
    inodeBitmap.byte[j] = inodeBitmap.byte[j] & tmp;
    //printf("j = %d, k = %d\n",j,k);
    
    //*inodeOffset = 0;
    diskWrite((void *)superBlock, sizeof(SuperBlock), 1, 0);
    diskWrite((void *)&inodeBitmap, sizeof(BlockBitmap), 1, inodeBitmapOffset * SECTOR_SIZE);

    return 0;
}

int freeInode (SuperBlock *superBlock, Inode *fatherInode, int fatherInodeOffset,
        Inode *destInode, int *destInodeOffset, const char *destFilename, int destFiletype) {
    int i = 0;
    int j = 0;
    int ret = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];
    int length = stringLen(destFilename);

    if (destFilename == NULL || destFilename[0] == 0)
        return -1;

    if (superBlock->availInodeNum == 0)
        return -1;
    
    for (i = 0; i < fatherInode->blockCount; i++) {
        ret = readBlock(superBlock, fatherInode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j++) {
            if (dirEntry[j].inode == 0)
                continue;
            else if (stringCmp(dirEntry[j].name, destFilename, length) == 0)
                break; // file with filename = destFilename exist
        }
        if (j < superBlock->blockSize / sizeof(DirEntry))
            break;
    }
    if (i == fatherInode->blockCount) {
        return -1;
    }
    
    *destInodeOffset = superBlock->inodeTable * SECTOR_SIZE + (dirEntry[j].inode - 1) * sizeof(Inode);  //To be remove
    diskRead((void*)destInode, sizeof(Inode), 1, *destInodeOffset);

    //Similar to the 'allocInode' function
    if(destInode->type != destFiletype){
        return -1;
    }
    destInode->linkCount--;
    //printf("    destInode->linkCount = %d\n",destInode->linkCount);
    if(destInode->linkCount == 0){  //Need to update the inodeBitmap,blockBitmap,etc.
        //Recall the block
        //printf("    destInode->blockCount = %d\n",destInode->blockCount);
        while(destInode->blockCount!=0){
            //Similiar to the 'allocLastBlock' function
            int divider0 = superBlock->blockSize / 4;
            int bound0 = POINTER_NUM;
            int bound1 = bound0 + divider0;
            int32_t singlyPointerBuffer[divider0];
            destInode->blockCount--;

            if (destInode->blockCount < bound0) {
                removeAllocedBlock(superBlock,&destInode->pointer[destInode->blockCount]);
            }
            else if (destInode->blockCount == bound0) {
                diskRead((void*)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, destInode->singlyPointer * SECTOR_SIZE);
                removeAllocedBlock(superBlock,&singlyPointerBuffer[0]);
                removeAllocedBlock(superBlock,&destInode->singlyPointer);
            }
            else if (destInode->blockCount < bound1) {
                diskRead((void*)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, destInode->singlyPointer * SECTOR_SIZE);
                removeAllocedBlock(superBlock,&singlyPointerBuffer[destInode->blockCount - bound0]);
            }
            else
                return -1;

            diskWrite((void*)destInode, sizeof(Inode), 1, *destInodeOffset);
        }
        //Recall the inode
        removeAllocedInode(superBlock,destInodeOffset);

        dirEntry[j].inode = 0; //set empty
    }
    else{  //write back
        diskWrite((void*)destInode, sizeof(Inode), 1, *destInodeOffset);
    }

    ret = writeBlock(superBlock, fatherInode, i, buffer);
    if (ret == -1)
        return -1;
    return 0;
}

int getAvailBlock (SuperBlock *superBlock, int *blockOffset) {
    int j = 0;
    int k = 0;
    int blockBitmapOffset = 0;
    BlockBitmap blockBitmap;

    if (superBlock->availBlockNum == 0)
        return -1;
    superBlock->availBlockNum--;

    blockBitmapOffset = superBlock->blockBitmap;
    diskRead((void*)&blockBitmap, sizeof(BlockBitmap), 1, blockBitmapOffset * SECTOR_SIZE);
    for (j = 0; j < superBlock->blockNum / 8; j++) {
        if (blockBitmap.byte[j] != 0xff) {
            break;
        }
    }
    for (k = 0; k < 8; k++) {
        if ((blockBitmap.byte[j] >> (7 - k)) % 2 == 0) {
            break;
        }
    }
    blockBitmap.byte[j] = blockBitmap.byte[j] | (1 << (7 - k));
    
    *blockOffset = superBlock->blocks + ((j * 8 + k) * superBlock->blockSize / SECTOR_SIZE);

    //printf("j=%d  k=%d  blockOffset=%d bitmap=%d\n",j,k,*blockOffset,blockBitmap.byte[j]);

    diskWrite((void*)&superBlock, sizeof(SuperBlock), 1, 0);
    diskWrite((void*)&blockBitmap, sizeof(BlockBitmap), 1, blockBitmapOffset * SECTOR_SIZE);

    return 0;
}

int allocLastBlock (SuperBlock *superBlock, Inode *inode, int inodeOffset, int blockOffset) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];
    int singlyPointerBufferOffset = 0;

    if (inode->blockCount < bound0) {
        inode->pointer[inode->blockCount] = blockOffset;
    }
    else if (inode->blockCount == bound0) {
        getAvailBlock(superBlock, &singlyPointerBufferOffset);
        singlyPointerBuffer[0] = blockOffset;
        inode->singlyPointer = singlyPointerBufferOffset;
        diskWrite((void*)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, singlyPointerBufferOffset * SECTOR_SIZE);
    }
    else if (inode->blockCount < bound1) {
        diskRead((void*)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
        singlyPointerBuffer[inode->blockCount - bound0] = blockOffset;
        diskWrite((void*)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
    }
    else
        return -1;
    
    inode->blockCount++;
    diskWrite((void*)inode, sizeof(Inode), 1, inodeOffset);
    return 0;
}

int calNeededPointerBlocks (SuperBlock *superBlock, int blockCount) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    if (blockCount == bound0)
        return 1;
    else if (blockCount >= bound1)
        return -1;
    else
        return 0;
}

int allocBlock (SuperBlock *superBlock, Inode *inode, int inodeOffset) {
    int ret = 0;
    int blockOffset = 0;

    ret = calNeededPointerBlocks(superBlock, inode->blockCount);
    if (ret == -1)
        return -1;
    if (superBlock->availBlockNum < ret + 1)
        return -1;
    
    getAvailBlock(superBlock, &blockOffset);
    allocLastBlock(superBlock, inode, inodeOffset, blockOffset);
    return 0;
}

int writeBlock (SuperBlock *superBlock, Inode *inode, int blockIndex, uint8_t *buffer) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];

    if (blockIndex < bound0) {
        diskWrite((void*)buffer, sizeof(uint8_t), superBlock->blockSize, inode->pointer[blockIndex] * SECTOR_SIZE);
        return 0;
    }
    else if (blockIndex < bound1) {
        diskRead((void*)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
		diskWrite((void*)buffer, sizeof(uint8_t), superBlock->blockSize, singlyPointerBuffer[blockIndex - bound0] * SECTOR_SIZE);
        return 0;
    }
    else
        return -1;
}

int getDirEntry (SuperBlock *superBlock, Inode *inode, int dirIndex, DirEntry *destDirEntry) {
    int i = 0;
    int j = 0;
    int ret = 0;
    int dirCount = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];
    for (i = 0; i < inode->blockCount; i++) {
        ret = readBlock(superBlock, inode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j ++) {
            if (dirEntry[j].inode != 0) {
                if (dirCount == dirIndex)
                    break;
                else
                    dirCount ++;
            }
        }
        if (j < superBlock->blockSize / sizeof(DirEntry))
            break;
    }
    if (i == inode->blockCount)
        return -1;
    else {
        destDirEntry->inode = dirEntry[j].inode;
        stringCpy(dirEntry[j].name, destDirEntry->name, NAME_LENGTH);
        return 0;
    }
}