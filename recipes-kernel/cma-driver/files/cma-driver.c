#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

static struct device cma_dev;
static dma_addr_t dma_handle;
static void *vaddr;
static size_t cma_size = 1024 * 1024;

static void cma_dev_release(struct device *dev) {}

static ssize_t cma_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    size_t len = strlen((char *)vaddr);
    if (*ppos >= len) return 0;
    if (count > len - *ppos) count = len - *ppos;
    if (copy_to_user(buf, vaddr + *ppos, count)) return -EFAULT;
    *ppos += count;
    return count;
}

static ssize_t cma_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    size_t len = min(count, cma_size - 1);
    memset(vaddr, 0, cma_size);
    if (copy_from_user(vaddr, buf, len)) return -EFAULT;
    pr_info("CMA: Data written to memory: %s\n", (char *)vaddr);
    return len;
}

static const struct file_operations cma_fops = {
    .owner = THIS_MODULE,
    .read = cma_read,
    .write = cma_write,
};

static struct miscdevice cma_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "cma_test",
    .fops = &cma_fops,
};

static int __init cma_driver_init(void) {
    int ret;

    dev_set_name(&cma_dev, "cma_device");
    cma_dev.release = cma_dev_release;
    ret = device_register(&cma_dev);
    if (ret) return ret;

    cma_dev.coherent_dma_mask = DMA_BIT_MASK(32);
    cma_dev.dma_mask = &cma_dev.coherent_dma_mask;

    vaddr = dma_alloc_coherent(&cma_dev, cma_size, &dma_handle, GFP_KERNEL);
    if (!vaddr) {
        device_unregister(&cma_dev);
        return -ENOMEM;
    }

    ret = misc_register(&cma_misc);
    if (ret) {
        dma_free_coherent(&cma_dev, cma_size, vaddr, dma_handle);
        device_unregister(&cma_dev);
        return ret;
    }

    pr_info("CMA: Ready. Physical: %pad, Device: /dev/cma_test\n", &dma_handle);
    return 0;
}

static void __exit cma_driver_exit(void) {
    misc_deregister(&cma_misc);
    if (vaddr) dma_free_coherent(&cma_dev, cma_size, vaddr, dma_handle);
    device_unregister(&cma_dev);
    pr_info("CMA: Driver removed\n");
}

module_init(cma_driver_init);
module_exit(cma_driver_exit);
MODULE_LICENSE("GPL");
