#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t *)
#define PB2_INSERT _IOW(0x10, 0x32, int32_t *)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t *)
#define PB2_EXTRACT _IOR(0x10, 0x34, int32_t *)

struct pb2_set_type_arguments
{
    int32_t heap_size;
    int32_t heap_type;
};

struct obj_info
{
    int32_t heap_size;
    int32_t heap_type;
    int32_t root;
    int32_t last_inserted;
};

struct result 
{
    int32_t result;
    int32_t heap_size;
};


void print_heap_info(struct obj_info info)
{
    printf("heap_size: %d\n",info.heap_size);
    printf("heap_type: %d\n",info.heap_type);
    printf("root: %d\n",info.root);
    printf("last_inserted: %d\n",info.last_inserted);
}
int main()
{
    int fd,value,status;
    fd = open("/proc/partb_2_16CS30030",O_RDWR);
    if(fd<0){
        printf("Unable to open proc file\n");
        exit(1);
    }
    struct pb2_set_type_arguments type_arg;
    struct result res;
    type_arg.heap_type = 0;
    type_arg.heap_size = 5;
    status = ioctl(fd,PB2_SET_TYPE,&type_arg);
    printf("Heap initialised, status: %d\n",status);
    value = 7;
    status = ioctl(fd, PB2_INSERT,&value);
    printf("Inserted %d into heap, status:%d\n",value,status);
    value = 9;
    status = ioctl(fd,PB2_INSERT,&value);
    printf("Inserted %d into heap, status: %d\n",value,status);
    value = 18;
    status = ioctl(fd,PB2_INSERT,&value);
    printf("Inserted %d into heap, status: %d\n",value,status);
    struct obj_info info;
    status = ioctl(fd,PB2_GET_INFO,(struct obj_info*)&info);
    printf("Getting heap info,status:%d \n",status);
    print_heap_info(info);
    status = ioctl(fd,PB2_EXTRACT, (struct result*)&res);
    printf("Extracted %d, size %d, status: %d\n",res.result,res.heap_size,status);
    int temp;
    scanf("%d",&temp);
    value = 12;
    status = ioctl(fd,PB2_INSERT,(int*)&value);
    printf("Inserted %d into heap, status: %d\n",value,status);
    status = ioctl(fd,PB2_GET_INFO,(struct obj_info*)&info);
    printf("Getting heap info, status:%d \n",status);
    print_heap_info(info);
    status = ioctl(fd,PB2_EXTRACT, (struct result*)&res);
    printf("Extracted %d, size %d, status: %d\n",res.result,res.heap_size,status);
    printf("Enter some random number, will be discarded: ");
    close(fd);
    return 0;
}
