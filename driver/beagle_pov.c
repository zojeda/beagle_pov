#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define MODULE_NAME "beaglepov"
#define BASE_SHARED_PRU_MEM  0x4a310000UL
#define SHARED_PRU_MEM_SIZE  60
//#define SHARED_PRU_MEM_SIZE  12*1024

void __iomem *shared_pru_mem = 0;

bool ppm_mode = false;


char* P6 = "P6";

static ssize_t beagle_pov_write(struct file * file, const char __user * buf, 
                          size_t count, loff_t *ppos) {
    int bytes = count;
    
    char *kbuf;

    printk(KERN_INFO  "%s: requested writing %d bytes at pos %llu\n",MODULE_NAME, bytes, *ppos);

    if(ppm_mode) {
        kbuf = kmalloc(bytes, GFP_KERNEL);
        //checking header
        if(strncmp(P6, kbuf, 2) != 0) {
            printk(KERN_INFO  "%s: invalid magic number %s\n",MODULE_NAME, kbuf);
            return -EFAULT;
        }
        //skiping whitespaces
        kbuf+=2;
        while(isspace(*kbuf)) 
            kbuf++;
        //skipping comments
        if(*kbuf=='#') {
            while(*kbuf!='\n')
                kbuf++;
            kbuf++;
        }
    } else {
        if(SHARED_PRU_MEM_SIZE<count+*ppos)
            return -ENOMEM;
        else
            bytes = count;
        kbuf = kmalloc(bytes, GFP_KERNEL);
    }

    printk(KERN_INFO  "%s: writing %d bytes at pos %llu\n",MODULE_NAME, bytes, *ppos);
    //count = SHARED_PRU_MEM_SIZE;
    if(!kbuf)
        return -ENOMEM;
    if (copy_from_user(kbuf, buf, bytes)) {
        printk(KERN_INFO  "%s: could not copy %d bytes.\n",MODULE_NAME, bytes);
        return -EFAULT;
    }
    memcpy_toio(shared_pru_mem+*ppos, kbuf, bytes);
    wmb();

    *ppos += bytes;
    kfree(kbuf);
    return bytes;
}
static ssize_t beagle_pov_read(struct file * file, char __user * buf, 
                          size_t count, loff_t *ppos) {
    ssize_t bytes;

    void *kbuf;

    printk(KERN_INFO  "%s: requested reading %d bytes at pos %llu\n",MODULE_NAME, count, *ppos);

    if (*ppos>SHARED_PRU_MEM_SIZE) {
        printk(KERN_INFO  "%s: start reading beyond available memory \n",MODULE_NAME);
        return -EINVAL;
    }
    
    if(SHARED_PRU_MEM_SIZE<count+*ppos)
        bytes = SHARED_PRU_MEM_SIZE-*ppos;
    else
        bytes = count;

    printk(KERN_INFO  "%s: reading %d bytes at pos %llu\n",MODULE_NAME, bytes, *ppos);

    if(bytes == 0) {
        printk(KERN_INFO  "%s: reached eof \n",MODULE_NAME);
        return 0;
    }

    kbuf = kmalloc(bytes, GFP_KERNEL);
    if(!kbuf)
        return -ENOMEM;

    memcpy_fromio(kbuf, shared_pru_mem+*ppos, bytes);
    rmb();

    
    if(copy_to_user(buf, kbuf, bytes)) {
        printk(KERN_INFO  "%s: could not copy %d bytes.\n",MODULE_NAME, bytes);
        return -EFAULT;
    }
    kfree(kbuf);

    printk(KERN_INFO  "%s: read %d bytes\n", MODULE_NAME, bytes);
    *ppos += bytes;

    return bytes;
}

static const struct file_operations beagle_pov_ops = {
    .owner  = THIS_MODULE,
    .read   = beagle_pov_read,
    .write  = beagle_pov_write,
};

static struct miscdevice beagle_pov = {
    //let the kernel to pick a minor number for us
    MISC_DYNAMIC_MINOR, 
    
    //device name: it will be added by udev as /dev/beagle_pov
    MODULE_NAME, 
    
    //file operations defining the device behaviour
    &beagle_pov_ops
};   



static int __init beagle_pov_init(void) {
    int ret;
    if (!request_mem_region(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE, MODULE_NAME)) {
        printk(KERN_INFO  "%s: can't get I/O mem address 1x%lx\n",MODULE_NAME, BASE_SHARED_PRU_MEM);
        return -ENODEV;
    }
    shared_pru_mem = ioremap(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE);

    ret = misc_register(&beagle_pov);
    if(ret) {
        release_mem_region(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE);
        printk(KERN_ERR "Unable to register [beagle_pov] misc device \n");
    }
    return ret;
}

static void __exit beagle_pov_exit(void) {
    misc_deregister(&beagle_pov);

    iounmap(shared_pru_mem);
    release_mem_region(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE);
}


module_init(beagle_pov_init);
module_exit(beagle_pov_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zacarias F. Ojeda <zojeda@gmail.com>");
MODULE_DESCRIPTION("'beagle_pov' driver");
MODULE_VERSION("dev");


