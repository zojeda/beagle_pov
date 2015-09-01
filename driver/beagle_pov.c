#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define MODULE_NAME "beaglepov"
#define BASE_SHARED_PRU_MEM  0x4a310000UL
#define SHARED_PRU_MEM_SIZE  60*3
#define BASE_PRU0_MEM  0x4a300000UL
#define PRU_MEM_SIZE 8*1024 
#define GPIO1 0x4804C000
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_SETDATAOUT 0x194
#define GPIO_DATAOUT 0x13c
#define GPIO_OE 0x134

#define STEPPER_RESET_BIT 1<<17
//#define SHARED_PRU_MEM_SIZE  12*1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zacarias F. Ojeda <zojeda@gmail.com>");
MODULE_DESCRIPTION("'beagle_pov' driver");
MODULE_VERSION("dev");



char* P6 = "P6";

static inline void set_hi_gpio1(u32 pin);
static inline void set_low_gpio1(u32 pin);

static bool is_stepper_running(void);

/* This sysfs entry handles the stepper reset cmd*/
static ssize_t sys_stepper_reset_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
static ssize_t sys_stepper_reset_show(struct device* dev, struct device_attribute* attr, char* buf);

/* This sysfs entry handles the stepper init delay cmd*/
static ssize_t sys_stepper_init_delay_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
static ssize_t sys_stepper_init_delay_show(struct device* dev, struct device_attribute* attr, char* buf);

static DEVICE_ATTR(stepper_reset, S_IWUSR | S_IRUGO, sys_stepper_reset_show, sys_stepper_reset_store);
static DEVICE_ATTR(stepper_init_delay, S_IWUSR | S_IRUGO, sys_stepper_init_delay_show, sys_stepper_init_delay_store);

struct beagle_pov  {
    struct platform_device *pdev;
    void __iomem *shared_pru_mem;
    void __iomem *pru0_mem;
    void __iomem *gpio1;

    bool ppm_mode;

} bpov;  


static ssize_t beagle_pov_write(struct file * file, const char __user * buf, 
                          size_t count, loff_t *ppos) {
    int bytes = count;
    
    char *kbuf;

    printk(KERN_INFO  "%s: requested writing %d bytes at pos %llu\n",MODULE_NAME, bytes, *ppos);

    if(bpov.ppm_mode) {
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
    memcpy_toio(bpov.shared_pru_mem+*ppos, kbuf, bytes);
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

    memcpy_fromio(kbuf, bpov.shared_pru_mem+*ppos, bytes);
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

static struct miscdevice beagle_pov_device = {
    //let the kernel to pick a minor number for us
    MISC_DYNAMIC_MINOR, 
    //device name: it will be added by udev as /dev/beagle_pov
    MODULE_NAME, 
    //file operations defining the device behaviour
    &beagle_pov_ops
};   
    

static int beagle_pov_probe(struct platform_device *pdev) {
    int ret;
    u32 pins_oe;

    printk(KERN_INFO  "%s: probe\n",MODULE_NAME);

    if (!request_mem_region(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE, MODULE_NAME)) {
        printk(KERN_INFO  "%s: can't get I/O mem address 1x%lx\n",MODULE_NAME, BASE_SHARED_PRU_MEM);
        return -ENODEV;
    }
    bpov.shared_pru_mem = ioremap(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE);
    bpov.pru0_mem = ioremap(BASE_PRU0_MEM, PRU_MEM_SIZE);
    bpov.gpio1 = ioremap(GPIO1, 0x198);

    ret = misc_register(&beagle_pov_device); 
    printk(KERN_INFO  "%s: misc device registered\n",MODULE_NAME);
    if(ret) {
        release_mem_region(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE);
        printk(KERN_ERR "Unable to register [beagle_pov] misc device \n");
    }
    ret = device_create_file(beagle_pov_device.this_device, &dev_attr_stepper_reset);
    if (ret < 0) {
        printk(KERN_ERR "failed to create reset /sys endpoint \n");
    }
    ret = device_create_file(beagle_pov_device.this_device, &dev_attr_stepper_init_delay);
    if (ret < 0) {
        printk(KERN_ERR "failed to create reset /sys endpoint \n");
    }
    
    //set output pins:
    pins_oe = ioread32(bpov.gpio1+GPIO_OE);
    rmb();
    iowrite32(pins_oe || STEPPER_RESET_BIT, bpov.gpio1+GPIO_OE);
    wmb();

    return 0;
}

static int beagle_pov_remove(struct platform_device *pdev) {
    struct device *dev = beagle_pov_device.this_device;

    device_remove_file(dev, &dev_attr_stepper_reset);
    device_remove_file(dev, &dev_attr_stepper_init_delay);
    misc_deregister(&beagle_pov_device);

    iounmap(bpov.gpio1);
    iounmap(bpov.pru0_mem);
    iounmap(bpov.shared_pru_mem);
    release_mem_region(BASE_SHARED_PRU_MEM, SHARED_PRU_MEM_SIZE);
    return 0;
}



static const struct of_device_id beagle_pov_dt_ids[] = {
    { .compatible = "beagle_pov,beagle_pov", .data = NULL, },
    {},
};

MODULE_DEVICE_TABLE(of, beagle_pov_dt_ids);

static struct platform_driver beagle_pov_driver = {
    .driver = {
       .name   = "beagle_pov",
       .owner  = THIS_MODULE,
       .of_match_table = beagle_pov_dt_ids,
     },
    .probe  = beagle_pov_probe,
    .remove = beagle_pov_remove,
};



static int __init beagle_pov_init(void) {
        printk(KERN_INFO "beagle_pov loaded\n");
        platform_driver_register(&beagle_pov_driver);
        return 0;
}

static void __exit beagle_pov_exit(void) {
    printk(KERN_INFO "beagle_pov unloaded\n");
    platform_driver_unregister(&beagle_pov_driver);
}


module_init(beagle_pov_init);
module_exit(beagle_pov_exit);

static ssize_t sys_stepper_reset_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count) {
    if(sysfs_streq("1", buf)) 
     set_hi_gpio1(STEPPER_RESET_BIT);
    else if(sysfs_streq("0", buf))
     set_low_gpio1(STEPPER_RESET_BIT);
    else 
      return -EINVAL;
      
     return count;
};
static ssize_t sys_stepper_reset_show(struct device* dev, struct device_attribute* attr, char* buf) {
    if(is_stepper_running()) 
        sprintf(buf, "1\n");
    else
        sprintf(buf, "0\n");

    return 2;
};
static ssize_t sys_stepper_init_delay_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count) {
    u32 res = 0; 
    int retval = kstrtouint(buf, 10, &res);
    if(retval) 
        return retval; 
    iowrite32(res, bpov.pru0_mem);        
    printk(KERN_INFO  "%s: delay store: %u \n",MODULE_NAME, res);
    return count;
};
static ssize_t sys_stepper_init_delay_show(struct device* dev, struct device_attribute* attr, char* buf) {
    u32 val = ioread32(bpov.pru0_mem);
    rmb();
    printk(KERN_INFO  "%s: delay show : %u \n",MODULE_NAME, val);
    sprintf(buf, "%u\n", val);

    return strlen(buf);
};

static bool is_stepper_running() {
    u32 pins_out = ioread32(bpov.gpio1+GPIO_DATAOUT);
    rmb();
    return pins_out & STEPPER_RESET_BIT;
}


static inline void set_hi_gpio1(u32 pin) {
    iowrite32(pin, bpov.gpio1+GPIO_SETDATAOUT );
}

static inline void set_low_gpio1(u32 pin) {
    iowrite32(pin, bpov.gpio1+GPIO_CLEARDATAOUT );
}



