#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define MODULE_NAME "beaglepov"
#define BASE_SHARED_PRU_MEM  0x4a310000UL
#define SHARED_PRU_MEM_SIZE  1*1024

u32* share_pru_mem = 0;


static ssize_t beagle_pov_write(struct file * file, const char __user * buf, 
                          size_t count, loff_t *ppos) {
    int retval = count;
    
    void __iomem *address = (void __iomem *) share_pru_mem;
    char *kbuf = kmalloc(count, GFP_KERNEL);

    printk(KERN_INFO  "%s: writing %d bytes at pos %llu\n",MODULE_NAME, count, *ppos);
    //count = SHARED_PRU_MEM_SIZE;

    if(!kbuf)
        return -ENOMEM;
    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;

    printk(KERN_INFO  "%s: storing in memory %s\n",MODULE_NAME, kbuf);
    
    memcpy_toio(address, kbuf, count);
    wmb();

    *ppos += retval;
    kfree(kbuf);
    return retval;
}
static ssize_t beagle_pov_read(struct file * file, char __user * buf, 
                          size_t count, loff_t *ppos) {
    int retval = SHARED_PRU_MEM_SIZE;
    void __iomem *address = (void __iomem *) share_pru_mem;
    void *kbuf = kmalloc(count, GFP_KERNEL);

    printk(KERN_INFO  "%s: reading %d bytes at pos %llu\n",MODULE_NAME, count, *ppos);

    /*
     * We only support reading the whole string at once.
     */                   
    if (count < SHARED_PRU_MEM_SIZE)
        return -EINVAL;
    
    if(*ppos != 0) {
        printk(KERN_INFO  "%s: reached eof \n",MODULE_NAME);
        return 0;
    }
    
    count = SHARED_PRU_MEM_SIZE;

    if(!kbuf)
        return -ENOMEM;

    memcpy_fromio(kbuf, address, count);
    rmb();

    if (copy_to_user(buf, kbuf, count))
        retval = -EFAULT;
    kfree(kbuf);

    printk(KERN_INFO  "%s: read %d bytes\n", MODULE_NAME, retval);
    *ppos += retval;

    return retval;
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
        printk(KERN_INFO  "%s: can't get I/O mem address 0x%lx\n",MODULE_NAME, BASE_SHARED_PRU_MEM);
        return -ENODEV;
    }
    share_pru_mem = ioremap(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE);
    ret = misc_register(&beagle_pov);
    if(ret) {
        release_mem_region(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE);
        printk(KERN_ERR "Unable to register [beagle_pov] misc device \n");
    }
    return ret;
}

static void __exit beagle_pov_exit(void) {
    misc_deregister(&beagle_pov);

    iounmap((void __iomem *) share_pru_mem);
    release_mem_region(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE);
}


module_init(beagle_pov_init);
module_exit(beagle_pov_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zacarias F. Ojeda <zojeda@gmail.com>");
MODULE_DESCRIPTION("'beagle_pov' driver");
MODULE_VERSION("dev");


