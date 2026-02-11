# kernel_driver_test

# CPU Statistics Character Device Driver

A simple Linux kernel module that creates a character device `/dev/cpustat`  
and exposes live CPU-related information from kernel space.

This project demonstrates:

- Kernel module creation
- Character device registration
- Major number allocation
- VFS routing
- Safe user ↔ kernel memory transfer
- Access to CPU and process information

---

## ⚠ Requirements

You must run this on a Linux system.

Recommended:
- Ubuntu 22.04 (VM preferred)
- VirtualBox or VMware

Kernel modules cannot be built or loaded on native Windows.

---

## 1. Install Required Packages

Check kernel version:

```

uname -r

```

Install dependencies:

```

sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)

```

---

## 2. Project Structure

```

cpustat_driver/
│
├── cpustat.c
├── Makefile
└── README.md

````

---

## 3. Source Code

### cpustat.c

```c
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
````

---

## 4. Makefile

```make
obj-m += cpustat.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
```

---

## 5. Build the Module

Inside project directory:

```
make
```

If successful, this generates:

```
cpustat.ko
```

---

## 6. Load the Module

```
sudo insmod cpustat.ko
```

Check logs:

```
dmesg | tail
```

You will see:

```
cpustat loaded: major=XXX
```

Note the major number.

---

## 7. Create Device File

Replace `XXX` with the major number from dmesg.

```
sudo mknod /dev/cpustat c XXX 0
sudo chmod 666 /dev/cpustat
```

---

## 8. Test the Driver

```
cat /dev/cpustat
```

Example Output:

```
CPU Statistics
--------------
Uptime (jiffies): 1234567
Current PID: 4211
CPU Core: 2
```

---

## 9. Remove the Module

```
sudo rmmod cpustat
```

Verify:

```
lsmod | grep cpustat
```

---

## 10. Clean Build Files

```
make clean
```

---

## What This Project Demonstrates

* Kernel module lifecycle (init/exit)
* Character device registration
* Major number allocation
* VFS → driver routing
* Access to scheduler context
* Multi-core awareness
* Kernel ↔ user memory safety

---

## Important Notes

* Always test in a virtual machine.
* Incorrect kernel code can crash the system.
* Ensure kernel headers match running kernel.

---

## Future Improvements

* Add write() handler
* Add ioctl() support
* Add wait queues for blocking read
* Auto-create device node using class_create()
* Extend to expose per-CPU stats

```

---

If you want, I can also give you:

- A more production-style README  
- A version with auto `/dev` creation (no manual `mknod`)  
- Or an upgraded version with `ioctl` support
```
