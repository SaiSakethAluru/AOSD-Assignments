/*
Team details:
Sai Saketh Aluru 16CS30030
G Chandan Ritvik 16CS30010
*/
#include "disk.h"


disk *create_disk(int nbytes)
{
    if(nbytes < BLOCKSIZE+24){
        printf("Given %d bytes not sufficient for the blocksize\n",nbytes);
        return NULL;
    }
    int num_blocks = (nbytes-24)/BLOCKSIZE;
    disk *new_disk = (disk*)malloc(sizeof(disk));
    new_disk->size = nbytes;
    new_disk->blocks = num_blocks;
    new_disk->reads = 0;
    new_disk->writes = 0;
    new_disk->block_arr = (char**) malloc(num_blocks * sizeof(char*));
    int i;
    for(i=0;i<num_blocks;i++){
        new_disk->block_arr[i] = (char*)malloc(BLOCKSIZE);
    }
    return new_disk;
}

int read_block(disk *diskptr, int blocknr, void *block_data)
{
    if(diskptr == NULL){
        return -1;
    }
    if(blocknr < 0 || blocknr >= diskptr->blocks){
        return -1;
    }
    memcpy(block_data,diskptr->block_arr[blocknr],BLOCKSIZE);
    diskptr->reads++;
    return 0;
}

int write_block(disk *diskptr, int blocknr, void *block_data)
{
    if(diskptr == NULL){
        return -1;
    }
    if(blocknr < 0 || blocknr >= diskptr->blocks){
        return -1;
    }
    memcpy(diskptr->block_arr[blocknr],block_data,BLOCKSIZE);
    diskptr->writes++;
    return 0;
}

int free_disk(disk *diskptr)
{
    int i;
    for(int i=0;i<diskptr->blocks;i++)
    {
        free(diskptr->block_arr[i]);
    }
    free(diskptr->block_arr);
    free(diskptr);
}
