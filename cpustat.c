#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/smp.h>

#define DEVICE_NAME "cpustat"
#define BUF_SIZE 256

static dev_t dev;
static struct cdev cpustat_cdev;
static DEFINE_MUTEX(cpustat_lock);

static ssize_t cpustat_read(struct file *file, char __user *buf,
                            size_t count, loff_t *ppos)
{
    char kbuf[BUF_SIZE];
    int len;

    if (*ppos > 0)
        return 0;

    mutex_lock(&cpustat_lock);

    len = snprintf(kbuf, BUF_SIZE,
        "CPU Statistics\n"
        "--------------\n"
        "Uptime (jiffies): %lu\n"
        "Current PID: %d\n"
        "CPU Core: %u\n",
        jiffies,
        current->pid,
        smp_processor_id()
    );

    mutex_unlock(&cpustat_lock);

    if (copy_to_user(buf, kbuf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = cpustat_read,
};

static int __init cpustat_init(void)
{
    if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0)
        return -1;

    cdev_init(&cpustat_cdev, &fops);

    if (cdev_add(&cpustat_cdev, dev, 1) < 0)
        return -1;

    printk(KERN_INFO "cpustat loaded: major=%d\n", MAJOR(dev));
    return 0;
}

static void __exit cpustat_exit(void)
{
    cdev_del(&cpustat_cdev);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "cpustat unloaded\n");
}

module_init(cpustat_init);
module_exit(cpustat_exit);

MODULE_LICENSE("GPL");
