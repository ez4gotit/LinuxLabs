#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "int_stack"
#define CLASS_NAME "stack"
#define IOCTL_MAGIC 0xF5
#define IOCTL_SET_SIZE _IOW(IOCTL_MAGIC, 0, int)

MODULE_LICENSE("GPL");

static int major;
static struct class*  stack_class = NULL;
static struct device* stack_device = NULL;

static int *stack = NULL;
static int top = -1;
static int max_size = 0;

static struct mutex stack_mutex;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops =
{
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
    .unlocked_ioctl = dev_ioctl,
};

static int dev_open(struct inode *inodep, struct file *filep) {
    mutex_lock(&stack_mutex);
    if (!stack && max_size > 0) {
        stack = kmalloc(sizeof(int) * max_size, GFP_KERNEL);
        top = -1;
    }
    mutex_unlock(&stack_mutex);
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    int value;
    if (mutex_lock_interruptible(&stack_mutex)) return -ERESTARTSYS;
    if (top < 0) {
        mutex_unlock(&stack_mutex);
        return 0; // NULL (empty stack)
    }
    value = stack[top--];
    mutex_unlock(&stack_mutex);
    return copy_to_user(buffer, &value, sizeof(int)) ? -EFAULT : sizeof(int);
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    int value;
    if (len < sizeof(int)) return -EINVAL;
    if (copy_from_user(&value, buffer, sizeof(int))) return -EFAULT;
    if (mutex_lock_interruptible(&stack_mutex)) return -ERESTARTSYS;
    if (top >= max_size - 1) {
        mutex_unlock(&stack_mutex);
        return -ERANGE;
    }
    stack[++top] = value;
    mutex_unlock(&stack_mutex);
    return sizeof(int);
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) {
    int new_size;
    if (cmd != IOCTL_SET_SIZE)
        return -EINVAL;

    if (copy_from_user(&new_size, (int __user *)arg, sizeof(int)))
        return -EFAULT;

    if (new_size <= 0) return -EINVAL;

    mutex_lock(&stack_mutex);
    kfree(stack);
    max_size = new_size;
    stack = kmalloc(sizeof(int) * max_size, GFP_KERNEL);
    top = -1;
    mutex_unlock(&stack_mutex);

    return 0;
}

static int __init int_stack_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) return major;

    stack_class = class_create(THIS_MODULE, CLASS_NAME);
    stack_device = device_create(stack_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    mutex_init(&stack_mutex);

    return 0;
}

static void __exit int_stack_exit(void) {
    device_destroy(stack_class, MKDEV(major, 0));
    class_unregister(stack_class);
    class_destroy(stack_class);
    unregister_chrdev(major, DEVICE_NAME);
    kfree(stack);
    mutex_destroy(&stack_mutex);
}

module_init(int_stack_init);
module_exit(int_stack_exit);
