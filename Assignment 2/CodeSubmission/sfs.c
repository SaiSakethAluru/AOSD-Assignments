/*
Team details:
Sai Saketh Aluru 16CS30030
G Chandan Ritvik 16CS30010
*/


#include "disk.h"
#include "sfs.h"

// Global filesystem pointer
disk* fs_diskptr = NULL;

// Utility functions for max and min of two numbers
int min(int a, int b)
{
    return a<b? a:b;
}

int max(int a, int b)
{
    return a>b?a:b;
}

int format(disk *diskptr)
{
    // Calculate different sizes of inodes, datablocks, bitmaps, etc
    int N = diskptr->blocks;
    int M = N-1;
    int I = floor(0.1 * M);
    int IB = ceil((double)(I*128) / (double)(8*BLOCKSIZE));
    int R = M - I - IB;
    int DBB = ceil((double)R / (double)(8*BLOCKSIZE));
    int DB = R-DBB;
    // Allocate superblock and write it to memory
    super_block *block = (super_block*) malloc(sizeof(super_block));
    block->magic_number = 12345;
    block->blocks = M;
    block->inode_blocks = I;
    block->inodes = I*128;
    block->inode_bitmap_block_idx = 1;
    block->inode_block_idx = 1+ IB + DBB;
    block->data_block_bitmap_idx = 1 + IB;
    block->data_block_idx = 1 + IB + DBB + I;
    block->data_blocks = DB;
    int ret = write_block(diskptr,0,block);
    // Initialise and write inode bitmap
    int inode_bitmap_len = ceil((double)block->inodes / (double)(8*sizeof(int)));
    int inode_bitmap[inode_bitmap_len];
    int i = 0, inode_block = block->inode_bitmap_block_idx;
    for(i=0;i<inode_bitmap_len;i++){
        inode_bitmap[i] = 0;
    }
    i=0;
    while(inode_block < 1+IB){
        ret |= write_block(diskptr,inode_block,&inode_bitmap[i]);
        inode_block++;
        i += BLOCKSIZE/sizeof(int);
    }
    // Initialise and write data bitmap
    int data_bitmap_len = ceil((double)R / 8*sizeof(int));
    int data_bitmap[data_bitmap_len];
    i=0;
    int data_block = block->data_block_bitmap_idx;
    for(i=0;i<data_bitmap_len;i++){
        data_bitmap[i] = 0;
    }
    i=0;
    while(data_block < 1+IB+DBB){
        ret |= write_block(diskptr,data_block,&data_bitmap[i]);
        data_block++;
        i += BLOCKSIZE/sizeof(int);
    }
    // Initialise all inodes with valid = 0 and write them.
    inode inodes[I*128];
    for(i=0;i<I*128;i++){
        inodes[i].valid = 0;
    }
    inode_block = block->inode_block_idx;
    i=0;
    while(inode_block < block->data_block_idx){
        ret |= write_block(diskptr,inode_block,&inodes[i]);
        inode_block++;
        i += BLOCKSIZE / sizeof(inode);
    }
    free(block);
    return ret;
}

int mount(disk *diskptr)
{
    // Load super block
    char buffer[BLOCKSIZE];
    if(read_block(diskptr,0,buffer)!=0){
        return -1;
    }
    super_block* sblock;
    memcpy(sblock,buffer,sizeof(super_block));
    // Check magic number. store the pointer if matches.
    if(sblock->magic_number != MAGIC){
        return -1;
    }
    fs_diskptr = diskptr;
    return 0;
}

int create_file()
{
    // Check if fs is mounted or not.
    if(fs_diskptr == NULL){
        return -1;
    }
    // load superblock to get starting indicies of different components of fs
    super_block* sblock;
    char buffer[BLOCKSIZE];
    read_block(fs_diskptr,0,buffer);
    memcpy(sblock,buffer,sizeof(super_block));
    // Check inode bitmap to look for first empty slot.
    int i=sblock->inode_bitmap_block_idx,j,k;
    int inode_bitmap_len = BLOCKSIZE / sizeof(int);
    int inode_bitmap[inode_bitmap_len];
    int inode_idx = 0;
    bool inode_found = false;
    while(!inode_found && i<sblock->data_block_bitmap_idx){
        if(read_block(fs_diskptr,i,inode_bitmap)!=0){
            return -1;
        }
        for(j=0;j<inode_bitmap_len;j++){
            for(k=31;k>=0;k++){
                int mask = 1<<k;
                if(mask&inode_bitmap[j]){
                    inode_idx++;
                }
                else{
                    inode_found=true;
                    inode_bitmap[j] |= mask;
                    break;
                } 
            }
            if(inode_found)
                break;
        }
        if(inode_found){
            if(write_block(fs_diskptr,i,inode_bitmap)!=0){
                return -1;
            }
        }
        i++;
    }
    // No more free inode location
    if(!inode_found){
        printf("No space for new inode\n");
        return -1;
    }
    // Load the block of inodes containing the free location
    int inode_arr_len = BLOCKSIZE/sizeof(inode);
    inode inodes[inode_arr_len];
    int inode_blk_idx = sblock->inode_block_idx +  inode_idx/128;
    if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    // Initialise the inode as valid with size = 0 and write back
    inodes[inode_idx%128].size = 0;
    inodes[inode_idx%128].valid = 1;
    if(write_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return 0;
    }
    return inode_idx;
}

int remove_file(int number)
{
    // Check if fs is mounted or not
    if(fs_diskptr == NULL){
        return -1;
    }
    // Load super block to get starting indicies of inodes and data bitmaps and blocks.
    super_block* sblock;
    char buffer[BLOCKSIZE];
    if(read_block(fs_diskptr,0,buffer)!=0){
        return -1;
    }
    memcpy(sblock,buffer,sizeof(super_block));
    // Check if given inode number is valid  or not
    if(number>sblock->inodes){
        printf("Invalid inode number given\n");
        return -1;
    }
    // Load the inode block containing the required inode.
    int inode_blk_idx = sblock->inode_block_idx + number/128;
    int inode_arr_len = BLOCKSIZE/sizeof(inode);
    inode inodes[inode_arr_len];
    if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    // Mark the file's size as 0, and make the inode invalid.
    int file_size = inodes[number%128].size;
    inodes[number%128].size = 0;
    inodes[number%128].valid = 0;
    // Write the modified inode to disk
    if(write_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    // Load the inode bitmap block containing the bitmap of the inode to be removed
    int inode_bitmap_len = ceil((double)sblock->inodes / (double)(8*sizeof(int)));
    int inode_bitmap_blk_idx = number / (8*BLOCKSIZE);
    int inode_bitmap[inode_bitmap_len];
    if(read_block(fs_diskptr,inode_bitmap_blk_idx,inode_bitmap)!=0){
        return -1;
    }
    // Unset the bit --> mark the inode block as free
    int inode_bitmap_arr_idx = (number%(8*BLOCKSIZE)) / 32;
    int mask = 1<<(31-number%32);
    mask = ~mask;
    inode_bitmap[inode_bitmap_arr_idx] &= mask;
    // write the modified inode bitmap back to disk
    if(write_block(fs_diskptr,inode_bitmap_blk_idx,inode_bitmap)!=0){
        return -1;
    }
    int R = sblock->blocks - sblock->inode_blocks - sblock->data_block_bitmap_idx;
    int data_bitmap_len = ceil((double)R / 8*sizeof(int));
    int data_bitmap[data_bitmap_len];
    bool datamap_done = false;
    // Mark the file's data block bitmaps as free
    // Direct data block pointers in the file are freed first. Keep reducing the file after clearing each direct pointer. 
    // Stop if all allocated ones are removed
    for(int i=0;i<5;i++){
        if(file_size<=0){
            datamap_done = true;
            break;
        }
        // Get the block that contains the bitmap of the direct pointer block
        int datablock_idx = inodes[number%128].direct[i];
        int databitmap_blk_idx = sblock->data_block_bitmap_idx + datablock_idx / (8*BLOCKSIZE);
        if(read_block(fs_diskptr,databitmap_blk_idx,data_bitmap)!=0){
            return -1;
        }
        // Mark the corresponding bit as 0
        int data_bitmap_arr_idx = (datablock_idx%(8*BLOCKSIZE))/32;
        int mask = 1<<(31-datablock_idx%32);
        mask = ~mask;
        data_bitmap[data_bitmap_arr_idx] &= mask;
        if(write_block(fs_diskptr,databitmap_blk_idx,data_bitmap)!=0){
            return -1;
        }
        // Reduce the file size after removing one block
        file_size -= BLOCKSIZE;
    }
    // If the file also spans across indirect pointers, need to deallocate them as well. 
    if(!datamap_done){
        // Load the block containing data bitmap of indirect pointer.
        int indirect_blk_idx = inodes[number%128].indirect;
        int indirect_databitmap_blk_idx = sblock->data_block_bitmap_idx + indirect_blk_idx/(8*BLOCKSIZE);
        if(read_block(fs_diskptr,indirect_databitmap_blk_idx,data_bitmap)!=0 ){
            return -1;
        }
        // Mark as free. 
        int data_bitmap_arr_idx = indirect_blk_idx / 32;
        int mask = 1<<(31-indirect_blk_idx%32);
        mask =  ~mask;
        data_bitmap[data_bitmap_arr_idx] &= mask;
        // Write the updated bitmap block. 
        if(write_block(fs_diskptr,indirect_databitmap_blk_idx,data_bitmap)!=0 ){
            return -1;
        }
        // Read all the indirect pointers
        int indirect_ptrs[1024];
        if(read_block(fs_diskptr,inodes[number%128].indirect,indirect_ptrs)!=0){
            return -1;
        }
        // Check the file size and mark each of the indirect blocks as invalid. 
        int i;
        for(i=0;i<1024;i++){
            if(file_size<0){
                break;
            }
            // Read the corresponding bitmap of the indirect data block
            int indirect_data_bitmap_blk_idx = sblock->data_block_bitmap_idx + indirect_ptrs[i] / (8*BLOCKSIZE);
            if(read_block(fs_diskptr,indirect_data_bitmap_blk_idx,data_bitmap)!=0){
                return -1;
            }
            // Mark as free.
            int data_bitmap_arr_idx = indirect_ptrs[i] / 32;
            int mask = 1<<(31-indirect_ptrs[i]%32);
            mask = ~mask;
            data_bitmap[data_bitmap_arr_idx] &= mask;
            // Write the modified block back into disk.
            if(write_block(fs_diskptr,indirect_data_bitmap_blk_idx,data_bitmap)!=0){
                return -1;
            }
            file_size -= BLOCKSIZE;
        }
    }
    return 0;
}

int stat(int inumber)
{
    // Read the superblock to check the inode counts and start indices.
    if(fs_diskptr == NULL){
        return -1;
    }
    super_block* sblock;
    char buffer[BLOCKSIZE];
    if(read_block(fs_diskptr,0,buffer)!=0){
        return -1;
    }
    memcpy(sblock,buffer,sizeof(super_block));
    // If inode number is invalid, return error.
    if(inumber> sblock->inodes){
        printf("Invalid inode number\n");
        return -1;
    }
    // Read the block containing the corresponding inode number.
    int inode_blk_idx = sblock->inode_block_idx + inumber/128;
    int inode_arr_len = BLOCKSIZE/sizeof(inode);
    inode inodes[inode_arr_len];
    if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    inode file_node = inodes[inumber%128];
    // Print the various statistics.
    printf("Statistics about file with inode number %d\n",inumber);
    printf("File size: %d\n",file_node.size);
    int direct = (file_node.size < BLOCKSIZE*5)? ceil((double)file_node.size/(double)BLOCKSIZE) : 5;
    int indirect = 0;
    if(file_node.size > 5*BLOCKSIZE){
        indirect  = ceil((double)(file_node.size-5*BLOCKSIZE) / (double)BLOCKSIZE);
    }
    printf("Number of direct pointers in use %d\n",direct);
    printf("Number of indirect pointers in use %d\n",indirect);
    int num_data_blocks = direct;
    if(indirect > 0){
        num_data_blocks += (1+indirect);
    }
    printf("Number of datablocks in use: %d\n",num_data_blocks);
    return 0;
}

int read_i(int number, char *data, int length, int offset)
{
    // Read the superblock to determine inode and data block indices and counts
    if(fs_diskptr == NULL){
        return -1;
    }
    super_block* sblock;
    char buffer[BLOCKSIZE];
    if(read_block(fs_diskptr,0,buffer)!=0){
        return -1;
    }
    memcpy(sblock,buffer,sizeof(super_block));
    // If invalid inode number, return error
    if(number> sblock->inodes){
        printf("Invalid inode number\n");
        return -1;
    }
    // Read the inode block containing required inode
    int inode_blk_idx = sblock->inode_block_idx + number/128;
    int inode_arr_len = BLOCKSIZE/sizeof(inode);
    inode inodes[inode_arr_len];
    if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    inode file_node = inodes[number%128];
    // If offset to read from greater than file size, error
    if(offset > file_node.size){
        printf("Trying to read with offset greater than file size\n");
        return -1;
    }
    int data_read = 0;
    int data_available = file_node.size - offset;
    // Can read either upto length number of bytes, or till file end, whichever is minimum
    int data_to_read = min(length,data_available);
    // If the offset is within the direct pointers, start from there. 
    if(offset < 5*BLOCKSIZE) {
        int direct_i = offset/BLOCKSIZE;
        while(direct_i < 5 && data_to_read > 0){
            // Read the direct block containing the offset point
            if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                return -1;
            }
            // Copy the data from offset to buffer. 
            memcpy(data+data_read, &buffer[offset%BLOCKSIZE],min(data_to_read,BLOCKSIZE-offset));
            // Update amount of data left to read and data read so far. 
            data_read += min(data_to_read,BLOCKSIZE-offset);
            data_to_read -= min(data_to_read,BLOCKSIZE-offset);
            direct_i++;
            // After first time reading, offset will become zero from next block onwards.
            offset = 0;
        }
        // If we need to read from indirect pointers as well
        if(data_to_read>0){
            int indirect_ptrs[1024];
            // Load the indirect pointers.
            if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                return -1;
            }
            int indirect_i=0;
            while(indirect_i<1024 && data_to_read > 0){
                // Read indirect pointers.
                if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                    return -1;
                }
                // Copy to the given buffer and update data read and to_read. 
                memcpy(data+data_read,buffer,min(data_to_read,BLOCKSIZE));
                data_read += min(data_to_read,BLOCKSIZE);
                data_to_read -= min(data_to_read,BLOCKSIZE);
                indirect_i++;
            }
        }
    }
    // If offset begins from indirect pointers. skip the direct pointer blocks, and adjust offset accordingly
    else{
        offset -= (5*BLOCKSIZE);
        int indirect_ptrs[1024];
        // Load the indirect pointer addresses.
        if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
            return -1;
        }
        int indirect_i = (offset) / BLOCKSIZE;
        // Read the indirect data blocks and copy to buffer.
        while(indirect_i < 1024 && data_to_read > 0){
            if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                return -1;
            }
            memcpy(data+data_read,&buffer[offset%BLOCKSIZE],min(data_to_read,BLOCKSIZE-offset));
            data_read += min(data_to_read,BLOCKSIZE-offset);
            data_to_read -=  min(data_to_read,BLOCKSIZE-offset);
            offset = 0;
            indirect_i++;
        }
    }
    return data_read;
}

int write_i(int inumber, char* data, int length, int offset)
{
    // Load the super block to get counts and indicies of data and inode blocks
    if(fs_diskptr == NULL){
        return -1;
    }
    super_block* sblock;
    char buffer[BLOCKSIZE];
    if(read_block(fs_diskptr,0,buffer)!=0){
        return -1;
    }
    memcpy(sblock,buffer,sizeof(super_block));
    // Invalid inode number passed
    if(inumber> sblock->inodes){
        printf("Invalid inode number\n");
        return -1;
    }
    // Load the inode of the file to write to
    int inode_blk_idx = sblock->inode_block_idx + inumber/128;
    int inode_arr_len = BLOCKSIZE/sizeof(inode);
    inode inodes[inode_arr_len];
    if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    inode file_node = inodes[inumber%128];
    // Can't write to a location beyond the file size. 
    if(offset > file_node.size){
        printf("Provided offset is more than the file size\n");
        return -1;
    }
    int data_written = 0;
    int max_available = (5+BLOCKSIZE/4)*BLOCKSIZE - offset;
    // We can write either length amount of data, or till file occupies all direct and indirect blocks, whichever is minimum.
    int data_to_write = min(length,max_available);
    // Count the number of direct and indirect pointers already allocated. Useful to know if we are overwriting existing ones or need to allocate new ones.
    int direct_ptrs_set = ceil( (double) min(file_node.size,5*BLOCKSIZE) /(double) BLOCKSIZE);
    int indirect_ptrs_set = ceil((double) max((file_node.size - 5*BLOCKSIZE),0)/(double)BLOCKSIZE);
    int data_bitmap_len = BLOCKSIZE / sizeof(int);
    int data_bitmap[data_bitmap_len];
    // If offset occurs in direct pointers
    if(offset < 5*BLOCKSIZE) {
        int direct_i = offset/BLOCKSIZE;
        while(direct_i < 5 && data_to_write > 0){
            // When offset is not 0, it means we are writing to an allocated block, so no need to allocate new. 
            if(offset != 0){
                // Load the existing block first. This is to preserve data before the offset. 
                if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                    return -1;
                }
                // Copy the data from buffer to appropriate location, keeping in mind the offset and amount of data that can be written in this block.
                memcpy(buffer+offset%BLOCKSIZE,data+data_written,min(data_to_write,BLOCKSIZE-offset%BLOCKSIZE));
                if(write_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                    return -1;
                }
                // Update values of data written and left to write.
                data_written += min(data_to_write,BLOCKSIZE-offset%BLOCKSIZE);
                data_to_write -=  min(data_to_write,BLOCKSIZE-offset%BLOCKSIZE);
                direct_i++;                
                // After first write, offset will become zero from next block onwards
                offset = 0;
            }
            // For offset = 0 case, have to check if we need to allocate new datablock
            else{
                // If data block already allocated, directly copy to the block
                if(direct_i < direct_ptrs_set){
                    memcpy(buffer,data+data_written,min(data_to_write,BLOCKSIZE));
                    if(write_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                        return -1;
                    }
                    data_written += min(data_to_write,BLOCKSIZE);
                    data_to_write -= min(data_to_write,BLOCKSIZE);
                    direct_i++;
                    offset = 0;
                }
                // If data block not allocated, look for new data blocks and allocate first free one found.
                else{
                    int data_blk_idx = 0;
                    bool data_blk_found = false;
                    int i=sblock->data_block_bitmap_idx;
                    while(!data_blk_found && i < sblock->inode_block_idx){
                        // Read the datablock bitmap to look for empty ones.
                        if(read_block(fs_diskptr,i,data_bitmap)!=0){
                            return -1;
                        }
                        int j,k;
                        for(j=0;j<data_bitmap_len;j++){
                            for(k=31;k>=0;k++){
                                int mask = 1<<k;
                                // If data block already occupied
                                if(mask & data_bitmap[j]){
                                    data_blk_idx++;
                                }
                                // Empty datablock found
                                else{
                                    data_blk_found = true;
                                    data_bitmap[j] |= mask;
                                    break;
                                }
                            }
                        }
                        if(data_blk_found){
                            break;
                            if(write_block(fs_diskptr,i,data_bitmap)!=0){
                                return -1;
                            }
                        }
                        i++;
                    }
                    // If no free blocks are available, written how much data was written so far
                    if(!data_blk_found){
                        printf("Memory full. no more data blocks available\n");
                        return data_written;
                    }
                    // Mark the alloted block in the file's inode
                    file_node.direct[direct_i] = data_blk_idx+sblock->data_block_idx;
                    // Write the modified inode to disk
                    memcpy(buffer,data+data_written,min(data_to_write,BLOCKSIZE));
                    if(write_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                        return -1;
                    }
                    data_written += min(data_to_write,BLOCKSIZE);
                    data_to_write -= min(data_to_write,BLOCKSIZE);
                    direct_i++;
                    offset = 0;
                }
            }
        }
        if(data_to_write > 0){
            // set new indirect block and additional pointers.
            // Allot data block to maintain the indirect pointer first
            if(indirect_ptrs_set == 0){
                int data_blk_idx = 0;
                bool data_blk_found = false;
                int i=sblock->data_block_bitmap_idx;
                while(!data_blk_found && i < sblock->inode_block_idx){
                    if(read_block(fs_diskptr,i,data_bitmap)!=0){
                        return -1;
                    }
                    int j,k;
                    for(j=0;j<data_bitmap_len;j++){
                        for(k=31;k>=0;k++){
                            int mask = 1<<k;
                            if(mask & data_bitmap[j]){
                                data_blk_idx++;
                            }
                            else{
                                data_blk_found = true;
                                data_bitmap[j] |= mask;
                                break;
                            }
                        }
                    }
                    if(data_blk_found){
                        break;
                        if(write_block(fs_diskptr,i,data_bitmap)!=0){
                            return -1;
                        }
                    }
                    i++;
                }
                // If no additional blocks left
                if(!data_blk_found){
                    printf("Memory full. no more data blocks available\n");
                    return data_written;
                }
                file_node.indirect = data_blk_idx+sblock->data_block_idx;
                int indirect_ptrs[1024];
                int indirect_i = 0;
                // Allot the indirect data pointers
                while(indirect_i < 1024 && data_to_write > 0){
                    int data_blk_idx = 0;
                    bool data_blk_found = false;
                    int i=sblock->data_block_bitmap_idx;
                    while(!data_blk_found && i < sblock->inode_block_idx){
                        if(read_block(fs_diskptr,i,data_bitmap)!=0){
                            return -1;
                        }
                        int j,k;
                        for(j=0;j<data_bitmap_len;j++){
                            for(k=31;k>=0;k++){
                                int mask = 1<<k;
                                if(mask & data_bitmap[j]){
                                    data_blk_idx++;
                                }
                                else{
                                    data_blk_found = true;
                                    data_bitmap[j] |= mask;
                                    break;
                                }
                            }
                        }
                        if(data_blk_found){
                            break;
                            if(write_block(fs_diskptr,i,data_bitmap)!=0){
                                return -1;
                            }
                        }
                        i++;
                    }
                    if(!data_blk_found){
                        return data_written;
                    }
                    // Copyt the data to the newly alloted indirect data block. 
                    indirect_ptrs[indirect_i] = data_blk_idx+sblock->data_block_idx;
                    memcpy(buffer,data+data_written,min(data_to_write,BLOCKSIZE));
                    if(write_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                        return -1;
                    }
                    data_written += min(data_to_write,BLOCKSIZE);
                    data_to_write -= min(data_to_write,BLOCKSIZE);
                }
                // Write the array of indirect pointers to the disk
                if(write_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                    return -1;
                }
            }
            // If indirect pointers array block is already set, only need to check for any new blocks needed
            else{
                int indirect_ptrs[1024];
                // Read the array of indirect pointers
                if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                    return -1;
                }
                int indirect_i = 0;
                while(indirect_i<1024 && data_to_write > 0){
                    // If we are overwriting an already allocated indirect data block
                    if(indirect_i <= indirect_ptrs_set){
                        memcpy(buffer, data+data_written,min(data_to_write,BLOCKSIZE));
                        if(write_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                            return -1;
                        }
                        data_written += min(data_to_write,BLOCKSIZE);
                        data_to_write -= min(data_to_write,BLOCKSIZE);
                        indirect_i++;
                    }
                    // If we need to allocate new block instead
                    else{
                        int data_blk_idx = 0;
                        bool data_blk_found = false;
                        int i=sblock->data_block_bitmap_idx;
                        while(!data_blk_found && i < sblock->inode_block_idx){
                            if(read_block(fs_diskptr,i,data_bitmap)!=0){
                                return -1;
                            }
                            int j,k;
                            for(j=0;j<data_bitmap_len;j++){
                                for(k=31;k>=0;k++){
                                    int mask = 1<<k;
                                    if(mask & data_bitmap[j]){
                                        data_blk_idx++;
                                    }
                                    else{
                                        data_blk_found = true;
                                        data_bitmap[j] |= mask;
                                        break;
                                    }
                                }
                            }
                            if(data_blk_found){
                                break;
                                if(write_block(fs_diskptr,i,data_bitmap)!=0){
                                    return -1;
                                }
                            }
                            i++;
                        }
                        // If no data block found
                        if(!data_blk_found){
                            printf("Memory full. no more data blocks available\n");
                            return data_written;
                        }
                        indirect_ptrs[indirect_i] = data_blk_idx+sblock->data_block_idx;
                        memcpy(buffer, data+data_written,min(data_to_write,BLOCKSIZE));
                        if(write_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                            return -1;
                        }
                        data_written += min(data_to_write,BLOCKSIZE);
                        data_to_write -= min(data_to_write,BLOCKSIZE);
                        indirect_i++;
                    }
                }
                // Write the array of pointers 
                if(write_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                    return -1;
                }
            }
        }
    }
    // If offset begins in the indirect pointers. It means the array of pointers already allocated
    else{
        int indirect_ptrs[1024];
        // Read the pointer array
        if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
            return -1;
        }
        int indirect_i = (offset - 5*BLOCKSIZE) / BLOCKSIZE;
        while(indirect_i < 1024 && data_to_write > 0){
            // If offset !=0, we are partially overwriting an existing block. 
            if(offset != 0){
                // Read the block first to preserve data before offset
                if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                    return -1;
                }
                // Modify the data in the block with appropriate offset adjustments
                memcpy(buffer+(offset-5*BLOCKSIZE)%BLOCKSIZE,data+data_written,min(data_to_write,BLOCKSIZE - offset%BLOCKSIZE));
                data_written += min(data_to_write,BLOCKSIZE - offset%BLOCKSIZE);
                data_to_write -= min(data_to_write,BLOCKSIZE - offset%BLOCKSIZE);
                offset = 0;
            }
            // If offset is 0, need to check if we are overwriting to old block or to a new block
            else{
                // If writing to already allocated block
                if(indirect_i<indirect_ptrs_set){
                    memcpy(buffer,data+data_to_write,min(data_to_write,BLOCKSIZE));
                    data_written += min(data_to_write,BLOCKSIZE);
                    data_to_write -= min(data_to_write,BLOCKSIZE);
                }
                // If new block needs to be given
                else{
                    int data_blk_idx = 0;
                    bool data_blk_found = false;
                    int i=sblock->data_block_bitmap_idx;
                    while(!data_blk_found && i < sblock->inode_block_idx){
                        if(read_block(fs_diskptr,i,data_bitmap)!=0){
                            return -1;
                        }
                        int j,k;
                        for(j=0;j<data_bitmap_len;j++){
                            for(k=31;k>=0;k++){
                                int mask = 1<<k;
                                if(mask & data_bitmap[j]){
                                    data_blk_idx++;
                                }
                                else{
                                    data_blk_found = true;
                                    data_bitmap[j] |= mask;
                                    break;
                                }
                            }
                        }
                        if(data_blk_found){
                            break;
                            // Write the modified data bitmap to disk
                            if(write_block(fs_diskptr,i,data_bitmap)!=0){
                                return -1;
                            }
                        }
                        i++;
                    }
                    // If no more data blocks available
                    if(!data_blk_found){
                        printf("Memory full. no more data blocks available\n");
                        return data_written;
                    }
                    indirect_ptrs[indirect_i] = data_blk_idx+sblock->data_block_idx;
                    memcpy(buffer,data+data_written,min(data_to_write,BLOCKSIZE));
                    // Write the data to newly allocated disk block
                    if(write_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                        return -1;
                    }
                    data_written += min(data_to_write,BLOCKSIZE);
                    data_to_write -= min(data_to_write,BLOCKSIZE);
                }
            }
            indirect_i++;
        }
        // Write the array of pointers to disk
        if(write_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
            return -1;
        }        
    }
    // Modify the file size in the inode and write it to disk.
    file_node.size += data_written;
    inodes[inumber%128] = file_node;
    if(write_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    return data_written;
}

int read_file(char *filepath, char *data, int length, int offset)
{
    // Assuming filepath is absolute path, and we always start from root directory / which is given inode 0
    char delim[2] = "/";
    char* filename;
    // Check if fs is mounted
    if(fs_diskptr == NULL){
        return -1;
    }
    // Load superblock to verify inode and data blocks and indicies
    super_block* sblock;
    char buffer[BLOCKSIZE];
    if(read_block(fs_diskptr,0,buffer)!=0){
        return -1;
    }
    memcpy(sblock,buffer,sizeof(super_block));
    // Initially take inode number as 0 for the root directory
    int inumber = 0;
    int inode_blk_idx = sblock->inode_block_idx + inumber / 128;
    int inode_arr_len = BLOCKSIZE / sizeof(inode);
    inode inodes[inode_arr_len];
    // load the root directory inode
    if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    inode file_node = inodes[inumber];
    // If root is not initialised or is empty, return error
    if(!file_node.valid || !file_node.size==0){
        printf("Root directory is not created or is empty\n");
        return -1;
    }
    // Get the number of files in it based on its size
    int num_files_in_dir = file_node.size / sizeof(directory_entry);
    directory_entry dir_entries[num_files_in_dir];
    int entries_read = 0;
    int direct_i = 0;
    // Read the list of files and subdirectories
    for(direct_i=0;direct_i<5;direct_i++){
        if(entries_read < num_files_in_dir){
            if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                return -1;
            }
            memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
            entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
        }
        else break;
    }
    if(entries_read < num_files_in_dir){
        int indirect_ptrs[1024];
        if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
            return -1;
        }
        int indirect_i=0;
        for(indirect_i=0;indirect_i<1024;indirect_i++){
            if(entries_read < num_files_in_dir){
                if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                    return -1;
                }
                memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
            }
            else break;
        }
    }
    // Tokenizer the absolute path to get the subdirectories along the tree.
    filename = strtok(filepath,delim);
    // For each folder in the path to the file
    while(filename!=NULL){
        int i;
        bool file_found = false;
        // Search for the folder / file in its parents list of entries
        for(i=0;i<num_files_in_dir;i++){
            // If match found
            if(dir_entries[i].valid && strcmp(dir_entries[i].name,filename)==0){
                inumber = dir_entries[i].inode;
                // If the matched entry is a file
                if(dir_entries[i].type==0){
                    // Check if this is the end of the given path. 
                    filename = strtok(NULL,delim);
                    // If end, this is the file we want to read
                    if(filename == NULL){
                        return read_i(inumber,data,length,offset);
                    }
                    // Else given path is invalid since a file name is given as folder name
                    else return -1;
                }
                // If this is a directory along the path, load the inode of the directory
                inode_blk_idx = sblock->inode_block_idx + inumber/128;
                if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
                    return -1;
                }
                file_node = inodes[inumber];
                // From the loaded inode, get the size and from there number of files in this subdirectory
                num_files_in_dir = file_node.size / sizeof(directory_entry);
                directory_entry dir_entries[num_files_in_dir];
                int entries_read = 0;
                int direct_i = 0;
                // Read all the entries of the directory for the next iteration of the outer while loop
                for(direct_i=0;direct_i<5;direct_i++){
                    if(entries_read < num_files_in_dir){
                        if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                            return -1;
                        }
                        memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                        entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
                    }
                    else break;
                }
                if(entries_read < num_files_in_dir){
                    int indirect_ptrs[1024];
                    if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                        return -1;
                    }
                    int indirect_i=0;
                    for(indirect_i=0;indirect_i<1024;indirect_i++){
                        if(entries_read < num_files_in_dir){
                            if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                                return -1;
                            }
                            memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                            entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
                        }
                        else break;
                    }
                }
                file_found = true;
                break;
            }
        }
        // If any intermediate or final subdirectory / file is not found
        if(!file_found){
            printf("File / folder not found\n");
            return -1;
        }
        filename = strtok(NULL,delim);
    }
    // error case shouldn't reach here.
    return -1;
}


int write_file(char *filepath, char *data, int length, int offset)
{
    // Assuming filepath is absolute path, and we always start from root directory / which is given inode 0
    char delim[2] = "/";
    char* filename;
    if(fs_diskptr == NULL){
        return -1;
    }
    // Load superblock to verify inode and data blocks and indicies
    super_block* sblock;
    char buffer[BLOCKSIZE];
    if(read_block(fs_diskptr,0,buffer)!=0){
        return -1;
    }
    memcpy(sblock,buffer,sizeof(super_block));
    // Initially take inode number as 0 for the root directory
    int inumber = 0;
    int inode_blk_idx = sblock->inode_block_idx + inumber / 128;
    int inode_arr_len = BLOCKSIZE / sizeof(inode);
    // load the root directory inode
    inode inodes[inode_arr_len];
    if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    inode file_node = inodes[inumber];
    // If root directory is not initialised or is empty, return error
    // Since root is a directory we can't use write to initialise a new file
    if(!file_node.valid || !file_node.size==0){
        return -1;
    }
    // Get the number of files in it based on its size
    int num_files_in_dir = file_node.size / sizeof(directory_entry);
    directory_entry dir_entries[num_files_in_dir+1];
    int entries_read = 0;
    int direct_i = 0;
    // Read the list of files and subdirectories
    for(direct_i=0;direct_i<5;direct_i++){
        if(entries_read < num_files_in_dir){
            if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                return -1;
            }
            memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
            entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
        }
        else break;
    }
    if(entries_read < num_files_in_dir){
        int indirect_ptrs[1024];
        if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
            return -1;
        }
        int indirect_i=0;
        for(indirect_i=0;indirect_i<1024;indirect_i++){
            if(entries_read < num_files_in_dir){
                if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                    return -1;
                }
                memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
            }
            else break;
        }
    }
    // Tokenizer the absolute path to get the subdirectories along the tree.
    filename = strtok(filepath,delim);
    // For each folder in the path to the file
    while(filename!=NULL){
        int i;
        bool file_found = false;
        // Search for the folder / file in its parents list of entries
        for(i=0;i<num_files_in_dir;i++){
            // If match found
            if(dir_entries[i].valid && strcmp(dir_entries[i].name,filename)==0){
                inumber = dir_entries[i].inode;
                // If the matched entry is a file
                if(dir_entries[i].type==0){
                    // Check if this is the end of the given path. 
                    filename = strtok(NULL,delim);
                    if(filename == NULL){
                    // If end, this is the file we want to write to
                        return write_i(inumber,data,length,offset);
                    }
                    // Else given path is invalid since a file name is given as folder name
                    else return -1;
                }
                // If this is a directory along the path, load the inode of the directory
                inode_blk_idx = sblock->inode_block_idx + inumber/128;
                if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
                    return -1;
                }
                // From the loaded inode, get the size and from there number of files in this subdirectory
                file_node = inodes[inumber];
                num_files_in_dir = file_node.size / sizeof(directory_entry);
                directory_entry dir_entries[num_files_in_dir+1];
                int entries_read = 0;
                int direct_i = 0;
                // Read all the entries of the directory for the next iteration of the outer while loop
                for(direct_i=0;direct_i<5;direct_i++){
                    if(entries_read < num_files_in_dir){
                        if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                            return -1;
                        }
                        memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                        entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
                    }
                    else break;
                }
                if(entries_read < num_files_in_dir){
                    int indirect_ptrs[1024];
                    if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                        return -1;
                    }
                    int indirect_i=0;
                    for(indirect_i=0;indirect_i<1024;indirect_i++){
                        if(entries_read < num_files_in_dir){
                            if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                                return -1;
                            }
                            memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                            entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
                        }
                        else break;
                    }
                }
                file_found = true;
                break;
            }
        }
        // If any intermediate or final subdirectory / file is not found
        if(!file_found){
            char newfile_name[100];
            strcpy(newfile_name,filename);
            // Check if this is the end of the absolute path. If not error since directory doesn't exist
            filename = strtok(NULL,delim);
            if(filename != NULL){
                printf("Invalid path of file.\n");
                return -1;
            }
            // If this is the end, then we need to create new file
            else{
                // Create file and add to the parent directory's list of files.
                int new_inode = create_file(), dir_entry_i;
                for(dir_entry_i=0;dir_entry_i<num_files_in_dir;dir_entry_i++){
                    if(!dir_entries[dir_entry_i].valid){
                        break;
                    }
                }
                if(dir_entry_i == num_files_in_dir)
                    num_files_in_dir++;
                // Initialise the directory entry as valid, with given filename
                dir_entries[dir_entry_i].valid=1;
                dir_entries[dir_entry_i].inode = new_inode;
                dir_entries[dir_entry_i].type = 0;
                strcpy(dir_entries[dir_entry_i].name,newfile_name);
                dir_entries[dir_entry_i].length = min(100,strlen(newfile_name));
                int entries_written = 0;
                int direct_i = 0;
                // Write back the directory data
                for(direct_i=0;direct_i<5;direct_i++){
                    if(entries_read < num_files_in_dir){
                        memcpy(buffer,&dir_entries[entries_read],min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir+1-entries_written));
                        if(write_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                            return -1;
                        }
                        entries_written += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir+1-entries_written);
                    }
                    else break;
                }
                if(entries_written < num_files_in_dir){
                    int indirect_ptrs[1024];
                    if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                        return -1;
                    }
                    int indirect_i=0;
                    for(indirect_i=0;indirect_i<1024;indirect_i++){
                        if(entries_written < num_files_in_dir+1){
                            memcpy(buffer,&dir_entries[entries_read],min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_written));
                            if(write_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                                return -1;
                            }
                            entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_written);
                        }
                        else break;
                    }
                }
                // Write to the newly created inode
                return write_i(new_inode,data,length,offset);
            }
        }
        filename = strtok(NULL,delim);
    }
    // error case shouldn't reach
    return -1;
}

int create_dir(char *dirpath)
{
    // Assuming directorypath is absolute path, and we always start from root directory / which is given inode 0
    char delim[2] = "/";
    char* filename;
    if(fs_diskptr == NULL){
        return -1;
    }
    // Load superblock to verify inode and data blocks and indicies
    super_block* sblock;
    char buffer[BLOCKSIZE];
    if(read_block(fs_diskptr,0,buffer)!=0){
        return -1;
    }
    memcpy(sblock,buffer,sizeof(super_block));
    // Initially take inode number as 0 for the root directory
    int inumber = 0;
    int inode_blk_idx = sblock->inode_block_idx + inumber / 128;
    int inode_arr_len = BLOCKSIZE / sizeof(inode);
    // load the root directory inode
    inode inodes[inode_arr_len];
    if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    inode file_node = inodes[inumber];
    // Root directory not initialised. Check if given path is the root path
    if(!file_node.valid){
        // If it is root, then modify the root directory as initialised.
        if(strcmp(dirpath,"/")==0){
            inodes[inumber].valid = 1;
            if(write_block(fs_diskptr,inode_blk_idx,inodes)!=0){
                return -1;
            }
        }
        else return -1;
    }
    // Get the number of files in it based on its size
    int num_files_in_dir = file_node.size / sizeof(directory_entry);
    directory_entry dir_entries[num_files_in_dir+1];
    int entries_read = 0;
    int direct_i = 0;
    // Read the list of files and subdirectories
    for(direct_i=0;direct_i<5;direct_i++){
        if(entries_read < num_files_in_dir){
            if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                return -1;
            }
            memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
            entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
        }
        else break;
    }
    if(entries_read < num_files_in_dir){
        int indirect_ptrs[1024];
        if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
            return -1;
        }
        int indirect_i=0;
        for(indirect_i=0;indirect_i<1024;indirect_i++){
            if(entries_read < num_files_in_dir){
                if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                    return -1;
                }
                memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
            }
            else break;
        }
    }
    // Tokenizer the absolute path to get the subdirectories along the tree.
    filename = strtok(dirpath,delim);
    char* dirname;
    dirname = filename;
    int parent_inode;
    // For each folder in the path to the file
    while(filename!=NULL){
        int i;
        bool file_found = false;
        // Search for the folder / file in its parents list of entries
        for(i=0;i<num_files_in_dir;i++){
            // If match found
            if(dir_entries[i].valid && (dir_entries[i].name,filename)==0){
                parent_inode = inumber;
                inumber = dir_entries[i].inode;
                // If the matched entry is a file, error
                if(dir_entries[i].type==0){
                    return -1;
                }
                // If this is a directory along the path, load the inode of the directory
                inode_blk_idx = sblock->inode_block_idx + inumber/128;
                if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
                    return -1;
                }
                file_node = inodes[inumber];
                num_files_in_dir = file_node.size / sizeof(directory_entry);
                directory_entry dir_entries[num_files_in_dir+1];
                int entries_read = 0;
                int direct_i = 0;
                // Read all the entries of the directory for the next iteration of the outer while loop
                for(direct_i=0;direct_i<5;direct_i++){
                    if(entries_read < num_files_in_dir){
                        if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                            return -1;
                        }
                        memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                        entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
                    }
                    else break;
                }
                if(entries_read < num_files_in_dir){
                    int indirect_ptrs[1024];
                    if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                        return -1;
                    }
                    int indirect_i=0;
                    for(indirect_i=0;indirect_i<1024;indirect_i++){
                        if(entries_read < num_files_in_dir){
                            if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                                return -1;
                            }
                            memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                            entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
                        }
                        else break;
                    }
                }
                file_found = true;
                break;
            }
        }
        // If no match found
        if(!file_found){
            dirname = filename;
            // check if this is last of the given path
            filename = strtok(NULL,delim);
            // If not return error
            if(filename!=NULL){
                return -1;
            }
            // Create new file for the directory. Search for a empty location in the parent's list
            int new_dir_inode = create_file(), dir_entry_i;
            for(dir_entry_i=0;dir_entry_i<num_files_in_dir;dir_entry_i++){
                if(!dir_entries[dir_entry_i].valid){
                    break;
                }
            }
            if(dir_entry_i == num_files_in_dir)
                num_files_in_dir++;
            // Initialise the directory entry as valid, with given folder name
            dir_entries[dir_entry_i].valid = 1;
            dir_entries[dir_entry_i].inode = new_dir_inode;
            dir_entries[dir_entry_i].type = 1;
            strcpy(dir_entries[dir_entry_i].name,dirname);
            dir_entries[dir_entry_i].length = min(100,strlen(dirname));
            int entries_written = 0;
            int direct_i = 0;
            // Write back the directory data
            for(direct_i=0;direct_i<5;direct_i++){
                if(entries_read < num_files_in_dir){
                    memcpy(buffer,&dir_entries[entries_read],min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir+1-entries_written));
                    if(write_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                        return -1;
                    }
                    entries_written += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir+1-entries_written);
                }
                else break;
            }
            if(entries_written < num_files_in_dir){
                int indirect_ptrs[1024];
                if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                    return -1;
                }
                int indirect_i=0;
                for(indirect_i=0;indirect_i<1024;indirect_i++){
                    if(entries_written < num_files_in_dir+1){
                        memcpy(buffer,&dir_entries[entries_read],min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_written));
                        if(write_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                            return -1;
                        }
                        entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_written);
                    }
                    else break;
                }
            }

        }
        dirname = filename;
        filename=  strtok(NULL,delim);
    }
    // directory already exists
    return -1;
}

int remove_dir(char* dirpath)
{
    // Assuming directorypath is absolute path, and we always start from root directory / which is given inode 0
    char delim[2] = "/";
    char* filename;
    if(fs_diskptr == NULL){
        return -1;
    }
    // Load superblock to verify inode and data blocks and indicies
    super_block* sblock;
    char buffer[BLOCKSIZE];
    if(read_block(fs_diskptr,0,buffer)!=0){
        return -1;
    }
    memcpy(sblock,buffer,sizeof(super_block));
    // Initially take inode number as 0 for the root directory
    int inumber = 0;
    int inode_blk_idx = sblock->inode_block_idx + inumber / 128;
    int inode_arr_len = BLOCKSIZE / sizeof(inode);
    inode inodes[inode_arr_len];
    if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
        return -1;
    }
    inode file_node = inodes[inumber];
    // Root directory not initialised
    if(!file_node.valid){
        return -1;
    }
    // Get the number of files in it based on its size
    int num_files_in_dir = file_node.size / sizeof(directory_entry);
    directory_entry* dir_entries = (directory_entry*) malloc(num_files_in_dir * sizeof(directory_entry));
    int entries_read = 0;
    int direct_i = 0;
    // Read the list of files and subdirectories
    for(direct_i=0;direct_i<5;direct_i++){
        if(entries_read < num_files_in_dir){
            if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                return -1;
            }
            memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
            entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
        }
        else break;
    }
    if(entries_read < num_files_in_dir){
        int indirect_ptrs[1024];
        if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
            return -1;
        }
        int indirect_i=0;
        for(indirect_i=0;indirect_i<1024;indirect_i++){
            if(entries_read < num_files_in_dir){
                if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                    return -1;
                }
                memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
            }
            else break;
        }
    }
    // Tokenizer the absolute path to get the subdirectories along the tree.
    filename = strtok(dirpath,delim);
    // For each folder in the path to the file
    while(filename!=NULL){
        int i;
        bool file_found = false;
        // Search for the folder / file in its parents list of entries
        for(i=0;i<num_files_in_dir;i++){
            // If match found
            if(dir_entries[i].valid && strcmp(dir_entries[i].name,filename)==0){
                inumber = dir_entries[i].inode;
                // type is file
                if(dir_entries[i].type==0){
                    // Check if this is the end of path. If so make a call to remove the file
                    filename = strtok(NULL,delim);
                    if(filename == NULL){
                        dir_entries[i].valid = 0;
                        return remove_file(inumber);
                    }
                    else return -1;
                }
                // If this is a folder, go deeper into the path
                inode_blk_idx = sblock->inode_block_idx + inumber/128;
                if(read_block(fs_diskptr,inode_blk_idx,inodes)!=0){
                    return -1;
                }
                file_node = inodes[inumber];
                num_files_in_dir = file_node.size / sizeof(directory_entry);
                dir_entries = (directory_entry*)realloc(dir_entries,num_files_in_dir*sizeof(directory_entry));
                int entries_read = 0;
                int direct_i = 0;
                // Read all the entries of the directory for the next iteration of the outer while loop
                for(direct_i=0;direct_i<5;direct_i++){
                    if(entries_read < num_files_in_dir){
                        if(read_block(fs_diskptr,file_node.direct[direct_i],buffer)!=0){
                            return -1;
                        }
                        memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                        entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
                    }
                    else break;
                }
                if(entries_read < num_files_in_dir){
                    int indirect_ptrs[1024];
                    if(read_block(fs_diskptr,file_node.indirect,indirect_ptrs)!=0){
                        return -1;
                    }
                    int indirect_i=0;
                    for(indirect_i=0;indirect_i<1024;indirect_i++){
                        if(entries_read < num_files_in_dir){
                            if(read_block(fs_diskptr,indirect_ptrs[indirect_i],buffer)!=0){
                                return -1;
                            }
                            memcpy(&dir_entries[entries_read],buffer,min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read));
                            entries_read += min(BLOCKSIZE/sizeof(directory_entry),num_files_in_dir-entries_read);
                        }
                        else break;
                    }
                }
                file_found = true;
                break;
            }
        }
        if(!file_found){
            return -1;
        }
        filename = strtok(NULL,delim);
    }
    // After finding the inode of the last directory, recursively delete everything inside it.
    int dir_entry_i = 0;
    for(dir_entry_i=0;dir_entry_i<num_files_in_dir;dir_entry_i++){
        // If this is a file, make a call to remove_file with corresponding inode number
        if(dir_entries[dir_entry_i].valid && dir_entries[dir_entry_i].type==0){
            if(remove_file(dir_entries[dir_entry_i].inode)!=0){
                return -1;
            }
        }
        // If this is a subdirectory, make a recursive call to delete this. 
        // Subdirectory's absolute path is <given folder's absolute path> + / + <subdirectory name>
        else if(dir_entries[dir_entry_i].valid && dir_entries[dir_entry_i].type==1){
            char* subdirpath = strcat(dirpath,"/");
            subdirpath = strcat(subdirpath,dir_entries[dir_entry_i].name);
            if(remove_dir(subdirpath)!=0){
                return -1;
            }
        }
    }
    // Finally remove the inode of this directory. 
    if(remove_file(inumber)!=0){
        return -1;
    }
    free(dir_entries);
    return 0;
}