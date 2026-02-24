#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/ktime.h>

#define CMA_IOC_MAGIC  'c'
#define CMA_GET_PHYS_ADDR _IOR(CMA_IOC_MAGIC, 1, dma_addr_t)

static struct device cma_dev;
static dma_addr_t dma_handle;
static void *vaddr;
static size_t cma_size = 1024 * 1024;

static void cma_dev_release(struct device *dev) {}

static long cma_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case CMA_GET_PHYS_ADDR:
            if (copy_to_user((dma_addr_t __user *)arg, &dma_handle, sizeof(dma_handle)))
                return -EFAULT;
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static ssize_t cma_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    if (*ppos >= cma_size) return 0;
    if (count > cma_size - *ppos) count = cma_size - *ppos;
    if (copy_to_user(buf, vaddr + *ppos, count)) return -EFAULT;
    *ppos += count;
    return count;
}

static ssize_t cma_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    size_t len = min(count, cma_size);
    if (copy_from_user(vaddr, buf, len)) return -EFAULT;
    return len;
}

static const struct file_operations cma_fops = {
    .owner = THIS_MODULE,
    .read = cma_read,
    .write = cma_write,
    .unlocked_ioctl = cma_ioctl,
};

static struct miscdevice cma_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "cma_test",
    .fops = &cma_fops,
};

static int __init cma_driver_init(void) {
    int ret;
    ktime_t start, end;

    dev_set_name(&cma_dev, "cma_device");
    cma_dev.release = cma_dev_release;
    ret = device_register(&cma_dev);
    if (ret) return ret;

    cma_dev.coherent_dma_mask = DMA_BIT_MASK(64);
    cma_dev.dma_mask = &cma_dev.coherent_dma_mask;

    start = ktime_get();
    vaddr = dma_alloc_coherent(&cma_dev, cma_size, &dma_handle, GFP_KERNEL);
    end = ktime_get();

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

    pr_info("CMA: Allocated %zu bytes in %lld ns\n", cma_size, ktime_to_ns(ktime_sub(end, start)));
    pr_info("CMA: Virtual %p, Physical %pad\n", vaddr, &dma_handle);

    return 0;
}

static void __exit cma_driver_exit(void) {
    misc_deregister(&cma_misc);
    if (vaddr) dma_free_coherent(&cma_dev, cma_size, vaddr, dma_handle);
    device_unregister(&cma_dev);
}

module_init(cma_driver_init);
module_exit(cma_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Liron Avrhmov");
MODULE_DESCRIPTION("CMA Driver with IOCTL for ARM64");
