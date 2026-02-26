#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/mm.h>
#include <linux/slab.h>

/* IOCTL Definitions */
#define CMA_IOC_MAGIC  'c'
#define CMA_GET_PHYS_ADDR _IOR(CMA_IOC_MAGIC, 1, dma_addr_t)
#define CMA_RW_DATA       _IOWR(CMA_IOC_MAGIC, 2, struct cma_rw_args)

struct cma_rw_args {
    void __user *buf;    /* User-space buffer pointer */
    size_t size;         /* Number of bytes to transfer */
    size_t offset;       /* Offset within CMA memory */
    bool write;          /* true = Write to CMA, false = Read from CMA */
};

/* Driver Context */
static struct device cma_dev;
static dma_addr_t dma_handle;
static void *vaddr;
static size_t cma_size = 600UL * 1024 * 1024; /* 1024 * 1024 (1MB) to 600MB */

static void cma_dev_release(struct device *dev) {}

/**
 * cma_mmap - Maps CMA physical memory to user-space virtual address
 */
static int cma_mmap(struct file *filp, struct vm_area_struct *vma) {
    size_t size = vma->vm_end - vma->vm_start;

    if (size > cma_size)
        return -EINVAL;

    /* Map physical PFN to user VMA with default page protection */
    if (remap_pfn_range(vma, vma->vm_start, dma_handle >> PAGE_SHIFT,
                        size, vma->vm_page_prot)) {
        return -EAGAIN;
    }

    return 0;
}

/**
 * cma_ioctl - Handles control commands and data transfer
 */
static long cma_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct cma_rw_args rw;

    switch (cmd) {
        case CMA_GET_PHYS_ADDR:
            /* Return the physical DMA address to user-space */
            if (copy_to_user((dma_addr_t __user *)arg, &dma_handle, sizeof(dma_handle)))
                return -EFAULT;
            break;

        case CMA_RW_DATA:
            /* Custom IOCTL for reading/writing CMA memory */
            if (copy_from_user(&rw, (void __user *)arg, sizeof(rw)))
                return -EFAULT;

            if (rw.offset + rw.size > cma_size)
                return -EINVAL;

            if (rw.write) {
                if (copy_from_user(vaddr + rw.offset, rw.buf, rw.size))
                    return -EFAULT;
            } else {
                if (copy_to_user(rw.buf, vaddr + rw.offset, rw.size))
                    return -EFAULT;
            }
            break;

        default:
            return -ENOTTY;
    }
    return 0;
}

/**
 * Standard read/write/fops interface
 */
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
    .owner          = THIS_MODULE,
    .read           = cma_read,
    .write          = cma_write,
    .unlocked_ioctl = cma_ioctl,
    .mmap           = cma_mmap,
};

static struct miscdevice cma_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "cma_test",
    .fops  = &cma_fops,
};

/**
 * Module entry point
 */
static int __init cma_driver_init(void) {
    int ret;

    /* Initialize dummy device for DMA API */
    dev_set_name(&cma_dev, "cma_device");
    cma_dev.release = cma_dev_release;
    ret = device_register(&cma_dev);
    if (ret) return ret;

    /* Configure DMA masks for 64-bit ARM */
    cma_dev.coherent_dma_mask = DMA_BIT_MASK(64);
    cma_dev.dma_mask = &cma_dev.coherent_dma_mask;

    /* Request contiguous memory from CMA pool */
    vaddr = dma_alloc_coherent(&cma_dev, cma_size, &dma_handle, GFP_KERNEL);
    if (!vaddr) {
        device_unregister(&cma_dev);
        return -ENOMEM;
    }

    /* Register as a misc device node in /dev */
    ret = misc_register(&cma_misc);
    if (ret) {
        dma_free_coherent(&cma_dev, cma_size, vaddr, dma_handle);
        device_unregister(&cma_dev);
        return ret;
    }

    pr_info("CMA Driver Loaded. Phys: %pad, Virt: %p\n", &dma_handle, vaddr);
    return 0;
}

/**
 * Module exit point
 */
static void __exit cma_driver_exit(void) {
    misc_deregister(&cma_misc);
    if (vaddr) dma_free_coherent(&cma_dev, cma_size, vaddr, dma_handle);
    device_unregister(&cma_dev);
    pr_info("CMA Driver Unloaded\n");
}

module_init(cma_driver_init);
module_exit(cma_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Liron Avrhmov");
MODULE_DESCRIPTION("CMA Driver with MMAP and RW IOCTL");