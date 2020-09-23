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
    printk(KERN_ALERT "Initialisation of heap module started");
    heap_obj.type = 0;
    heap_obj.size = 0;
    heap_obj.max_size = 0;
    heap_obj.init = 0;
    heap_obj.arr = NULL;
    struct proc_dir_entry *entry = proc_create("partb_1_16CS30030", 0, NULL, &file_ops);
    if(!entry) return -ENOENT;
    file_ops.owner = THIS_MODULE; 
    file_ops.write = write;
    file_ops.read = read;
    printk(KERN_ALERT "Initialisation of heap module done");
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) { 
    // printk("%.*s", count, buf);
    int value;
    int valid = kstrtoint(buf,0,&value);
    if(!valid) return -EINVAL;
    if(heap_obj.init==0){
        if(value!=0xFF && value!=0xF0){
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
            heap_insert(value);
            return count;
        }
        else{
            return -EACCES;
        }
    }
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
    if(heap_obj.init != 2){
        return -1;
    }
    if(heap_obj.size==0){
        return -EACCES;
    }
    int value = heap_pop();
    if(copy_to_user(buf,&value,sizeof(value))){
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
    int i = heap_obj.size;
    heap_obj.size++;
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


