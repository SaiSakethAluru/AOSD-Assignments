#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t *)
#define PB2_INSERT _IOW(0x10, 0x32, int32_t *)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t *)
#define PB2_EXTRACT _IOR(0x10, 0x34, int32_t *)
#define PERMS 0666

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

MODULE_LICENSE("GPL");

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos);
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos);
static void heapify(int i,int index);
static int heap_pop(int index);
static void heap_insert(int value,int index);

static struct file_operations file_ops;

static char buffer[256] = {0};
static int buffer_len = 0;

static int lchild(int i)
{
    return 2 * i + 1;
}

static int rchild(int i)
{
    return 2 * i + 2;
}

static int parent(int i)
{
    return (i - 1) / 2;
}

typedef struct heap
{
    int type;
    int max_size;
    int size;
    int init;
    int last_insert;
    int *arr;
    int pid;
} heap;

static heap* heap_obj_arr;
static int heap_obj_arr_maxsize = 100;
static int heap_obj_arr_size = 0;

static int open(struct inode *inode, struct file* file)
{
    if(heap_obj_arr_size<heap_obj_arr_maxsize){
        heap_obj_arr[heap_obj_arr_size].type=0;
        heap_obj_arr[heap_obj_arr_size].size=0;
        heap_obj_arr[heap_obj_arr_size].max_size=0;
        heap_obj_arr[heap_obj_arr_size].init=0;
        heap_obj_arr[heap_obj_arr_size].last_insert=0;
        heap_obj_arr[heap_obj_arr_size].arr=NULL;
        heap_obj_arr[heap_obj_arr_size].pid=current->pid;
        heap_obj_arr_size++;
        return 0;
    }
    return -1;
}

static int close(struct inode *inode,struct file* file)
{
    int index,i;
    for(i=0;i<heap_obj_arr_size;i++){
        if(current->pid == heap_obj_arr[i].pid){
            index=i;
            break;
        }
    }   
    if(heap_obj_arr_size > 1){
        heap_obj_arr_size--;
        heap_obj_arr[index].type = heap_obj_arr[heap_obj_arr_size].type;
        heap_obj_arr[index].size = heap_obj_arr[heap_obj_arr_size].size;
        heap_obj_arr[index].max_size = heap_obj_arr[heap_obj_arr_size].max_size;
        heap_obj_arr[index].init = heap_obj_arr[heap_obj_arr_size].init;
        heap_obj_arr[index].last_insert = heap_obj_arr[heap_obj_arr_size].last_insert;
        if(heap_obj_arr[index].arr != NULL){
            kfree(heap_obj_arr[index].arr);
        }
        heap_obj_arr[index].arr = heap_obj_arr[heap_obj_arr_size].arr;
        if(heap_obj_arr[heap_obj_arr_size].arr != NULL){
            kfree(heap_obj_arr[heap_obj_arr_size].arr);
        }
        heap_obj_arr[index].pid = heap_obj_arr[heap_obj_arr_size].pid;
    }
    else{
        heap_obj_arr_size--;
        if(heap_obj_arr[index].arr != NULL){
            kfree(heap_obj_arr[index].arr);
        }
    }
    return 0;
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int index,i;
    for(i=0;i<heap_obj_arr_size;i++){
        if(current->pid == heap_obj_arr[i].pid){
            index=i;
            break;
        }
    }
    switch(cmd){
        case PB2_SET_TYPE:
            ;
            struct pb2_set_type_arguments* set_type_args = (struct pb2_set_type_arguments*) arg;
            if(set_type_args->heap_type !=0 && set_type_args->heap_type!=1){
                printk(KERN_ALERT "Init Invalid argument, %d, %d",set_type_args->heap_type,set_type_args->heap_size);
                return -EINVAL;
            }
            if(set_type_args->heap_size <1 || set_type_args->heap_size>100){
                printk(KERN_ALERT "Init invalid bounds of size");
                return -EINVAL;
            }
            heap_obj_arr[index].type = set_type_args->heap_type + 1;
            heap_obj_arr[index].max_size = set_type_args->heap_size;
            heap_obj_arr[index].init = 2;
            if(heap_obj_arr[index].arr != NULL){
                kfree(heap_obj_arr[index].arr);
            }
            heap_obj_arr[index].arr = (int*) kmalloc(heap_obj_arr[index].max_size * sizeof(int), GFP_KERNEL);
            return 0;

        case PB2_INSERT:
            if(heap_obj_arr[index].init!=2){
                printk(KERN_ALERT "insert not initialised\n");
                return -EACCES;
            }
            int *insert_value = (int*) arg;
            if(heap_obj_arr[index].size < heap_obj_arr[index].max_size){
                heap_insert(*insert_value,index);
                return 0;
            }
            printk(KERN_ALERT "insert heap full\n");
            return -EACCES;

        case PB2_GET_INFO:
            if(heap_obj_arr[index].init!=2){
                printk(KERN_ALERT "info not init\n");
                return -EACCES;
            }
            struct obj_info *info_args = (struct obj_info*) arg;
            info_args->heap_size = heap_obj_arr[index].size;
            info_args->heap_type = heap_obj_arr[index].type-1;
            info_args->root = (heap_obj_arr[index].size > 0? heap_obj_arr[index].arr[0]: 0);
            info_args->last_inserted = heap_obj_arr[index].last_insert;
            return 0;

        case PB2_EXTRACT:
            if(heap_obj_arr[index].init!=2){
                printk(KERN_ALERT "extract not init\n");
                return -EACCES;
            }
            struct result *result_args = (struct result*) arg;
            if(heap_obj_arr[index].size==0){
                printk(KERN_ALERT "extract empty heap\n");
                return -1;
            }
            int extract_value = heap_pop(index);
            result_args->result = extract_value;
            result_args->heap_size = heap_obj_arr[index].size;
            return 0;
        default:
            printk(KERN_ALERT "wrong command\n");
            return -EINVAL;
    }
    return 0;
} 
static int heap_init(void)
{
    printk(KERN_INFO "Initialisation of heap module started\n");
    struct proc_dir_entry *entry = proc_create("partb_2_16CS30030", PERMS, NULL, &file_ops);
    heap_obj_arr = (heap*) kmalloc(heap_obj_arr_maxsize * sizeof(heap),GFP_KERNEL);
    if (entry == NULL)
    {
        printk(KERN_ALERT "Error in creating proc entry\n");
        return -ENOENT;
    }
    file_ops.owner = THIS_MODULE;
    file_ops.write = write;
    file_ops.read = read;
    file_ops.unlocked_ioctl = ioctl;
    file_ops.open = open;
    file_ops.release = close;
    printk(KERN_INFO "Initialisation of heap module done\n");
    return 0;
}

static void heap_exit(void)
{
    printk(KERN_INFO "Removing heap module\n");
    kfree(heap_obj_arr);
    remove_proc_entry("partb_2_16CS30030", NULL);
    printk(KERN_INFO "Removal of module done\n");
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos)
{
    int index,i;
    for(i=0;i<heap_obj_arr_size;i++){
        if(current->pid == heap_obj_arr[i].pid){
            index=i;
            break;
        }
    }
    if (heap_obj_arr[index].init == 0)
    {
        char type,size;
        printk(KERN_INFO "RECEIVED %d %d\n",buf[0],buf[1]);
        if(count < 2){
            printk(KERN_ALERT "Invaild arguments for initialisation\n");
            return -EINVAL;
        }
        type = buf[0];
        // memcpy(&type,&buf[0],1);
        size = buf[1];
        // memcpy(&size,&buf[1],1);
        if(type != (char)0xff && type != (char)0xf0){
            printk(KERN_ALERT "First input to heap should be 0xFF or 0xF0, received %d\n",(unsigned int)type);
            return -EINVAL;
        }
        if((int) size < 1 || (int) size > 100){
            printk(KERN_ALERT "Size of heap is out of bounds, received %d\n", (unsigned int)size);
            heap_obj_arr[index].init = 0;
            heap_obj_arr[index].type = 0;
            return -EINVAL;
        }
        else{
            if(type == (char) 0xff){
                heap_obj_arr[index].type = 1;
            }
            else if( type == (char)0xf0){
                heap_obj_arr[index].type = 2;
            }
            heap_obj_arr[index].max_size = (int)size;
            heap_obj_arr[index].arr = (int *)kmalloc((heap_obj_arr[index].max_size)*sizeof(int),GFP_KERNEL);
            heap_obj_arr[index].init = 2;
            return count;
        }
        
    }
    else
    {
        int value;
        memcpy(&value,buf,count < sizeof(value)? count:sizeof(value));
        if (heap_obj_arr[index].size < heap_obj_arr[index].max_size)
        {
            printk(KERN_INFO "inserting %d into heap\n", value);
            heap_insert(value,index);
            printk(KERN_INFO "Heap size = %d\n", heap_obj_arr[index].size);
            return count<sizeof(value)?count:sizeof(value);
        }
        else
        {
            printk(KERN_ALERT "Heap is full, currently there are %d elements\n", heap_obj_arr[index].size);
            return -EACCES;
        }
    }
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos)
{
    int index,i;
    for(i=0;i<heap_obj_arr_size;i++){
        if(current->pid == heap_obj_arr[i].pid){
            index = i;
            break;
        }
    }
    printk(KERN_INFO "Heap init status: %d, Heap size: %d\n", heap_obj_arr[index].init, heap_obj_arr[index].size);
    printk(KERN_INFO "Attempting to read count: %ld\n", count);
    if (heap_obj_arr[index].init != 2)
    {
        printk(KERN_ALERT "Reading from heap without initialising\n");
        return -1;
    }
    if (heap_obj_arr[index].size == 0)
    {
        printk(KERN_ALERT "Reading from empty heap\n");
        return -EACCES;
    }
    int value = heap_pop(index);
    if (copy_to_user(buf, &value, sizeof(value)<count?sizeof(value):count) != 0)
    {
        printk(KERN_ALERT "Copy to buffer failed\n");
        return -1;
    }
    return sizeof(value)<count? sizeof(value):count;
}
static void heapify(int i,int index)
{
    if (heap_obj_arr[index].type == 1)
    {
        int smallest = (lchild(i) < heap_obj_arr[index].size && heap_obj_arr[index].arr[lchild(i)] < heap_obj_arr[index].arr[i]) ? lchild(i) : i;
        if (rchild(i) < heap_obj_arr[index].size && heap_obj_arr[index].arr[rchild(i)] < heap_obj_arr[index].arr[smallest])
        {
            smallest = rchild(i);
        }
        if (smallest != i)
        {
            int temp = heap_obj_arr[index].arr[smallest];
            heap_obj_arr[index].arr[smallest] = heap_obj_arr[index].arr[i];
            heap_obj_arr[index].arr[i] = temp;
            heapify(smallest,index);
        }
    }
    else if (heap_obj_arr[index].type == 2)
    {
        int largest = (lchild(i) < heap_obj_arr[index].size && heap_obj_arr[index].arr[lchild(i)] > heap_obj_arr[index].arr[i]) ? lchild(i) : i;
        if (rchild(i) < heap_obj_arr[index].size && heap_obj_arr[index].arr[rchild(i)] > heap_obj_arr[index].arr[largest])
        {
            largest = rchild(i);
        }
        if (largest != i)
        {
            int temp = heap_obj_arr[index].arr[largest];
            heap_obj_arr[index].arr[largest] = heap_obj_arr[index].arr[i];
            heap_obj_arr[index].arr[i] = temp;
            heapify(largest,index);
        }
    }
}

static int heap_pop(int index)
{
    heap_obj_arr[index].size--;
    int root = heap_obj_arr[index].arr[0];
    heap_obj_arr[index].arr[0] = heap_obj_arr[index].arr[heap_obj_arr[index].size];
    heapify(0,index);
    return root;
}
static void heap_insert(int value, int index)
{
    int i = heap_obj_arr[index].size;
    heap_obj_arr[index].size++;
    if (heap_obj_arr[index].type == 1)
    {
        while (i > 0 && value < heap_obj_arr[index].arr[parent(i)])
        {
            heap_obj_arr[index].arr[i] = heap_obj_arr[index].arr[parent(i)];
            i = parent(i);
        }
        heap_obj_arr[index].arr[i] = value;
    }
    else if (heap_obj_arr[index].type == 2)
    {
        while (i > 0 && value > heap_obj_arr[index].arr[parent(i)])
        {
            heap_obj_arr[index].arr[i] = heap_obj_arr[index].arr[parent(i)];
            i = parent(i);
        }
        heap_obj_arr[index].arr[i] = value;
    }
    heap_obj_arr[index].last_insert = value;
}

module_init(heap_init);
module_exit(heap_exit);