#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/slab.h>

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

MODULE_LICENSE("GPL");

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos);
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos);
static void heapify(int i);
static int heap_pop(void);
static void heap_insert(int value);

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
} heap;

static heap heap_obj;


static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch(cmd){
        case PB2_SET_TYPE:
            // if(!arg){
                // printk(KERN_ALERT "Init Invalid arg pointer\n");
                // return -EINVAL;
            // }
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
            heap_obj.type = set_type_args->heap_type + 1;
            heap_obj.max_size = set_type_args->heap_size;
            heap_obj.init = 2;
            if(heap_obj.arr != NULL){
                kfree(heap_obj.arr);
            }
            heap_obj.arr = (int*) kmalloc(heap_obj.max_size * sizeof(int), GFP_KERNEL);
            return 0;

        case PB2_INSERT:
            if(heap_obj.init!=2){
                printk(KERN_ALERT "insert not initialised\n");
                return -EACCES;
            }
            // if(!arg){
            //     printk(KERN_ALERT "insert invalid arg pointer\n");
            //     return -EINVAL;
            // }
            int *insert_value = (int*) arg;
            if(heap_obj.size < heap_obj.max_size){
                heap_insert(*insert_value);
                return 0;
            }
            printk(KERN_ALERT "insert heap full\n");
            return -EACCES;

        case PB2_GET_INFO:
            if(heap_obj.init!=2){
                printk(KERN_ALERT "info not init\n");
                return -EACCES;
            }
            // if(!arg){
            //     printk(KERN_ALERT "info invalid arg pointer\n");
            //     return -1;
            // }
            struct obj_info *info_args = (struct obj_info*) arg;
            info_args->heap_size = heap_obj.size;
            info_args->heap_type = heap_obj.type-1;
            info_args->root = (heap_obj.size > 0? heap_obj.arr[0]: 0);
            info_args->last_inserted = heap_obj.last_insert;
            return 0;

        case PB2_EXTRACT:
            if(heap_obj.init!=2){
                printk(KERN_ALERT "extract not init\n");
                return -EACCES;
            }
            // if(!arg){
            //     printk(KERN_ALERT "extract invalid arg pointer\n");
            //     return -1;
            // }
            struct result *result_args = (struct result*) arg;
            if(heap_obj.size==0){
                printk(KERN_ALERT "extract empty heap\n");
                return -1;
            }
            int extract_value = heap_pop();
            result_args->result = extract_value;
            result_args->heap_size = heap_obj.size;
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
    struct proc_dir_entry *entry = proc_create("partb_2_16CS30030", 0, NULL, &file_ops);
    heap_obj.type = 0;
    heap_obj.size = 0;
    heap_obj.max_size = 0;
    heap_obj.init = 0;
    heap_obj.last_insert = 0;
    heap_obj.arr = NULL;
    if (entry == NULL)
    {
        printk(KERN_ALERT "Error in creating proc entry\n");
        return -ENOENT;
    }
    file_ops.owner = THIS_MODULE;
    file_ops.write = write;
    file_ops.read = read;
    file_ops.unlocked_ioctl = ioctl;
    printk(KERN_INFO "Initialisation of heap module done\n");
    return 0;
}

static void heap_exit(void)
{
    printk(KERN_INFO "Removing heap module\n");
    heap_obj.type = 0;
    heap_obj.size = 0;
    heap_obj.max_size = 0;
    kfree(heap_obj.arr);
    remove_proc_entry("partb_2_16CS30030", NULL);
    printk(KERN_INFO "Removal of module done\n");
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos)
{
    int value;
    memcpy(buffer, buf, count);
    buffer[count] = '\0';
    int valid = kstrtoint(buffer, 0, &value);
    if (valid != 0)
    {
        printk(KERN_ALERT "Invalid argument passed to write, %s, %ld, %d\n", buffer, count, value);
        printk(KERN_INFO "Value received = %d\n", valid);
        return -EINVAL;
    }
    if (heap_obj.init == 0)
    {
        if (value != 0xFF && value != 0xF0)
        {
            printk(KERN_ALERT "First input to heap should be 0xFF or 0xF0, received %s, %ld, %d\n", buffer, count, value);
            return -EINVAL;
        }
        else
        {
            heap_obj.init = 1;
            if (value == 0xFF)
            {
                heap_obj.type = 1;
            }
            else if (value == 0xF0)
            {
                heap_obj.type = 2;
            }
            return count;
        }
    }
    else if (heap_obj.init == 1)
    {
        if (value < 1 || value > 100)
        {
            heap_obj.init = 0;
            heap_obj.type = 0;
            printk(KERN_ALERT "Size of heap is out of bounds, received %d\n", value);
            return -EINVAL;
        }
        else
        {
            heap_obj.init = 2;
            heap_obj.max_size = value;
            heap_obj.arr = (int *)kmalloc((heap_obj.max_size) * sizeof(int), GFP_KERNEL);
        }
        return count;
    }
    else
    {
        if (heap_obj.size < heap_obj.max_size)
        {
            printk(KERN_INFO "inserting %d into heap\n", value);
            heap_insert(value);
            printk(KERN_INFO "Heap size = %d\n", heap_obj.size);
            return count;
        }
        else
        {
            printk(KERN_ALERT "Heap is full, currently there are %d elements\n", heap_obj.size);
            return -EACCES;
        }
    }
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos)
{
    printk(KERN_INFO "Heap init status: %d, Heap size: %d\n", heap_obj.init, heap_obj.size);
    printk(KERN_INFO "Attempting to read count: %ld\n", count);
    if (heap_obj.init != 2)
    {
        printk(KERN_ALERT "Reading from heap without initialising\n");
        return -1;
    }
    if (heap_obj.size == 0)
    {
        printk(KERN_ALERT "Reading from empty heap\n");
        return -EACCES;
    }
    int value = heap_pop();
    buffer_len = sprintf(buffer, "%d", value);
    buffer[buffer_len] = '\0';
    if (copy_to_user(buf, buffer, buffer_len) != 0)
    {
        printk(KERN_ALERT "Copy to buffer failed\n");
        return -1;
    }
    int return_val = buffer_len;
    buffer_len = 0;
    return return_val;
}
static void heapify(int i)
{
    if (heap_obj.type == 1)
    {
        int smallest = (lchild(i) < heap_obj.size && heap_obj.arr[lchild(i)] < heap_obj.arr[i]) ? lchild(i) : i;
        if (rchild(i) < heap_obj.size && heap_obj.arr[rchild(i)] < heap_obj.arr[smallest])
        {
            smallest = rchild(i);
        }
        if (smallest != i)
        {
            int temp = heap_obj.arr[smallest];
            heap_obj.arr[smallest] = heap_obj.arr[i];
            heap_obj.arr[i] = temp;
            heapify(smallest);
        }
    }
    else if (heap_obj.type == 2)
    {
        int largest = (lchild(i) < heap_obj.size && heap_obj.arr[lchild(i)] > heap_obj.arr[i]) ? lchild(i) : i;
        if (rchild(i) < heap_obj.size && heap_obj.arr[rchild(i)] > heap_obj.arr[largest])
        {
            largest = rchild(i);
        }
        if (largest != i)
        {
            int temp = heap_obj.arr[largest];
            heap_obj.arr[largest] = heap_obj.arr[i];
            heap_obj.arr[i] = temp;
            heapify(largest);
        }
    }
}

static int heap_pop(void)
{
    heap_obj.size--;
    int root = heap_obj.arr[0];
    heap_obj.arr[0] = heap_obj.arr[heap_obj.size];
    heapify(0);
    return root;
}
static void heap_insert(int value)
{
    int i = heap_obj.size;
    heap_obj.size++;
    if (heap_obj.type == 1)
    {
        while (i > 0 && value < heap_obj.arr[parent(i)])
        {
            heap_obj.arr[i] = heap_obj.arr[parent(i)];
            i = parent(i);
        }
        heap_obj.arr[i] = value;
    }
    else if (heap_obj.type == 2)
    {
        while (i > 0 && value > heap_obj.arr[parent(i)])
        {
            heap_obj.arr[i] = heap_obj.arr[parent(i)];
            i = parent(i);
        }
        heap_obj.arr[i] = value;
    }
    heap_obj.last_insert = value;
}

module_init(heap_init);
module_exit(heap_exit);