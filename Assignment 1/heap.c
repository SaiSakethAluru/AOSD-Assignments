#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/slab.h>

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
    return 2*i+1;
}

static int rchild(int i)
{
    return 2*i+2;
}

static int parent(int i)
{
    return (i-1)/2;
}

typedef struct heap{
    int type;
    int max_size;
    int size;
    int init;
    int *arr;
}heap;

static heap heap_obj;

static int heap_init(void)
{
    printk(KERN_INFO "Initialisation of heap module started\n");
    struct proc_dir_entry *entry = proc_create("partb_1_16CS30030", 0, NULL, &file_ops);
    heap_obj.type = 0;
    heap_obj.size = 0;
    heap_obj.max_size = 0;
    heap_obj.init = 0;
    heap_obj.arr = NULL;
    if(entry==NULL) {
        printk(KERN_ALERT "Error in creating proc entry\n");
        return -ENOENT;
    }
    file_ops.owner = THIS_MODULE; 
    file_ops.write = write;
    file_ops.read = read;
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
    remove_proc_entry("partb_1_16CS30030",NULL);
    printk(KERN_INFO "Removal of module done\n");
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) { 
    int value;
    memcpy(buffer,buf,count);
    buffer[count] = '\0';
    
    int valid = kstrtoint(buffer,0,&value);
    if(valid!=0) {
        printk(KERN_ALERT "Invalid argument passed to write, %s, %ld, %d\n",buffer, count, value);
        printk(KERN_INFO "Value received = %d\n",valid);
        return -EINVAL;
    }
    if(heap_obj.init==0){
        if(value!=0xFF && value!=0xF0){
            printk(KERN_ALERT "First input to heap should be 0xFF or 0xF0, received %s, %ld, %d\n",buffer, count, value);
            return -EINVAL;
        }
        else{
            heap_obj.init = 1;
            if(value==0xFF){
                heap_obj.type = 1;
            }
            else if(value==0xF0){
                heap_obj.type = 2;
            }
            return count;
        }
    }
    else if(heap_obj.init==1){
        if(value<1 || value > 100){
            heap_obj.init = 0;
            heap_obj.type = 0;
            printk(KERN_ALERT "Size of heap is out of bounds, received %d\n",value);
            return -EINVAL;
        }
        else{
            heap_obj.init = 2;
            heap_obj.max_size = value;
            heap_obj.arr = (int*) kmalloc((heap_obj.max_size)*sizeof(int),GFP_KERNEL);
        }
        return count;
    }
    else{
        if(heap_obj.size<heap_obj.max_size){
            printk(KERN_INFO "inserting %d into heap\n",value);
            heap_obj.size++;
            heap_insert(value);
            printk(KERN_INFO "Heap size = %d\n",heap_obj.size);
            return count;
        }
        else{
            printk(KERN_ALERT "Heap is full, currently there are %d elements\n",heap_obj.size);
            return -EACCES;
        }
    }
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
    printk(KERN_INFO "Heap init status: %d, Heap size: %d\n",heap_obj.init,heap_obj.size);
    printk(KERN_INFO "Attempting to read count: %ld\n",count);
    if(heap_obj.init != 2){
        printk(KERN_ALERT "Reading from heap without initialising\n");
        return -1;
    }
    if(heap_obj.size==0){
        printk(KERN_ALERT "Reading from empty heap\n");
        return -EACCES;
    }
    int value = heap_pop();
    if(copy_to_user(buf,&value,sizeof(value))!=0){
        printk(KERN_ALERT "Copy to buffer failed\n");
        return -1;
    }
    return sizeof(value);
}
static void heapify(int i)
{
    if(heap_obj.type==1){
        int smallest = (lchild(i) < heap_obj.size && heap_obj.arr[lchild(i)] < heap_obj.arr[i])? lchild(i) : i;
        if(rchild(i) < heap_obj.size && heap_obj.arr[rchild(i)] < heap_obj.arr[smallest]){
            smallest = rchild(i);
        }
        if(smallest != i){
            int temp = heap_obj.arr[smallest];
            heap_obj.arr[smallest] = heap_obj.arr[i];
            heap_obj.arr[i] = temp;
            heapify(smallest);
        }
    }
    else if(heap_obj.type==2){
        int largest = (lchild(i) < heap_obj.size && heap_obj.arr[lchild(i)] > heap_obj.arr[i])? lchild(i) : i;
        if(rchild(i)< heap_obj.size && heap_obj.arr[rchild(i)] > heap_obj.arr[largest]){
            largest = rchild(i);
        }
        if(largest != i){
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
    int i = heap_obj.size-1;
    if(heap_obj.type==1){
        while(i>0 && value < heap_obj.arr[parent(i)]){
            heap_obj.arr[i] = heap_obj.arr[parent(i)];
            i = parent(i);
        }
        heap_obj.arr[i] = value;
    }
    else if(heap_obj.type==2){
        while(i>0 && value > heap_obj.arr[parent(i)]){
            heap_obj.arr[i] = heap_obj.arr[parent(i)];
            i = parent(i);
        }
        heap_obj.arr[i] = value;
    }
}

module_init(heap_init);
module_exit(heap_exit);