
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>


/* Platform Devices and Device Tree
http://lwn.net/Articles/448499/
http://lwn.net/Articles/448502/
*/

#define MAX_PRUS            2
/* PRU CONTROL */
#define PCTRL_CONTROL       0x0000
#define CONTROL_SOFT_RST_N  0x0001
#define CONTROL_ENABLE      0x0002

struct pruproc;
struct pruproc_core;


struct pruproc_core {
    int idx;
    struct pruproc *pruproc;
    u32 pctrl;
    u32 entry_point;
};

struct pruproc {
    struct platform_device *pdev;
    void __iomem *vaddr;
    dma_addr_t paddr;


    u32 num_prus;
    struct pruproc_core **pruc;
};


static inline void pru_write_reg(struct pruproc *pp, unsigned int reg,
        u32 val)
{
    __raw_writel(val, pp->vaddr + reg);
}

static inline void pcntrl_write_reg(struct pruproc_core *ppc, unsigned int reg,
        u32 val)
{
    return pru_write_reg(ppc->pruproc, reg + ppc->pctrl, val);
}

static ssize_t pruproc_reset(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct pruproc *pp = platform_get_drvdata(pdev);
    struct pruproc_core *ppc;
    int i;
    u32 val;
    
    /* first halt every PRU affected */
    for (i = 0; i < pp->num_prus; i++) {
        ppc = pp->pruc[i];
        /* keep it in reset */
        pcntrl_write_reg(ppc, PCTRL_CONTROL, CONTROL_SOFT_RST_N);
    }

    /* start every PRU affected */
    for (i = 0; i < pp->num_prus; i++) {
        ppc = pp->pruc[i];
        val = CONTROL_ENABLE | ((ppc->entry_point >> 2) << 16);
        pcntrl_write_reg(ppc, PCTRL_CONTROL, val);

    };

	printk(KERN_INFO "resetting \n");


    return strlen(buf);
}

static DEVICE_ATTR(reset, S_IWUSR, NULL, pruproc_reset);

static int pruproc_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *node = dev->of_node;
    struct device_node *pnode = NULL;
    struct pruproc *pp;
    struct pruproc_core *ppc;
    struct resource *res;
    u32 pru_idx;

    int err, i;
	printk(KERN_INFO "pruproc probe beagle_pov starting\n");

    pp = devm_kzalloc(dev, sizeof(*pp), GFP_KERNEL);
    if (pp == NULL) {
        dev_err(dev, "failed to allocate pruproc\n");
        err = -ENOMEM;
        goto err_fail;

    }

    /* link the device with the pruproc */
    platform_set_drvdata(pdev, pp);
    pp->pdev = pdev;


    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res == NULL) {
        dev_err(dev, "failed to parse MEM resource\n");
        goto err_fail;
    }

    pp->paddr = res->start;
    pp->vaddr = devm_ioremap(dev, res->start, resource_size(res));
    if (pp->vaddr == NULL) {
        dev_err(dev, "failed to parse MEM resource\n");
        goto err_fail;
    }


    /* count number of child nodes with a firmwary property */
    pp->num_prus = 0;
    for_each_child_of_node(node, pnode) {
        if (of_find_property(pnode, "firmware", NULL))
            pp->num_prus++;
    }
    pnode = NULL;

    /* found any? */
    if (pp->num_prus == 0) {
        dev_err(dev, "no pru nodes found\n");
        err = -EINVAL;
        goto err_fail;
    }
    /* found too many? */
    if (pp->num_prus > MAX_PRUS) {
        dev_err(dev, "Only 2 PRU nodes are supported\n");
        err = -EINVAL;
        goto err_fail;
    }
    dev_info(dev, "found #%d PRUs\n", pp->num_prus);

    /* allocate pointers */
    pp->pruc = devm_kzalloc(dev, sizeof(*pp->pruc) * pp->num_prus,
            GFP_KERNEL);
    if (pp->pruc == NULL) {
        dev_err(dev, "Failed to allocate PRU table\n");
        err = -ENOMEM;
        goto err_fail;
    }

    /* now iterate over all the pru nodes */
    i = 0;
    for_each_child_of_node(node, pnode) {

        /* only nodes with firmware are PRU nodes */
        if (of_find_property(pnode, "firmware", NULL) == NULL)
            continue;

        /* get the hardware index of the PRU */
        err = of_property_read_u32(pnode, "pru-index", &pru_idx);
        if (err != 0) {
            dev_err(dev, "can't find property %s\n", "pru-index");
            of_node_put(pnode);
            goto err_fail;
        }
        ppc->idx = pru_idx;
        ppc->pruproc = pp;
    }



    printk("Creating sysfs entries\n");
    
    err = device_create_file(dev, &dev_attr_reset);
    if (err != 0) {
        dev_err(dev, "device_create_file failed\n");
        goto err_fail;
    }

	printk(KERN_INFO "pruproc probe beagle_pov done\n");

err_fail:
	return 0;
}

/* PRU is unregistered */
static int pruproc_remove(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    device_remove_file(dev, &dev_attr_reset);
	return 0;
}

static const struct of_device_id beagle_pov_dt_ids[] = {
	{ .compatible = "beagle_pov,beagle_pov", .data = NULL, },
	{},
};

MODULE_DEVICE_TABLE(of, beagle_pov_dt_ids);

static struct platform_driver beagle_pov_driver = {
	.driver	= {
		.name	= "beagle_pov",
		.owner	= THIS_MODULE,
		.of_match_table = beagle_pov_dt_ids,
	},
	.probe	= pruproc_probe,
	.remove	= pruproc_remove,
};



static int __init beagle_pov_init(void)
{
	printk(KERN_INFO "beagle_pov loaded\n");
	platform_driver_register(&beagle_pov_driver);
	return 0;
}

static void __exit beagle_pov_exit(void)
{
	printk(KERN_INFO "beagle_pov unloaded\n");
	platform_driver_unregister(&beagle_pov_driver);
}



module_init(beagle_pov_init);
module_exit(beagle_pov_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Beagle POV driver");
MODULE_AUTHOR("Zacarias Ojeda <zojeda@gmail.com>");
