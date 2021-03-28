
#ifndef SDC_SHM_H
#define SDC_SHM_H

#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
* 共享内存设备，利用它分配进程间零拷贝传递的共享内存
* 也可以利用它来分配MMZ地址，MMZ地址分配之后的使用和共享内存完全一样
*/
#define SDC_SHM_CACHE_DEV "/dev/cache"

struct sdc_shm_cache
{
    void* addr_virt;
    unsigned long addr_phy;
    unsigned int size;
    unsigned int cookie;
};

#define SDC_FT_CACHE 7

/** 这是分配用户态进程之间可以零拷贝传递的共享内存 */
#define SDC_CACHE_ALLOC _IOR(SDC_FT_CACHE,0x00, struct sdc_shm_cache)

/** 这是分配用户态进程和海思芯片之间交换数据的MMZ内存 */
#define SDC_MMZ_ALLOC _IOR(SDC_FT_CACHE,0x10, struct sdc_shm_cache)

/** 当前事件接口需要用户调用MMAP */
#define SDC_CACHE_MMAP _IOR(SDC_FT_CACHE,0x01, struct sdc_shm_cache)

/** 释放分配的零拷贝内存（包括MMZ) */
#define SDC_CACHE_FREE _IOR(SDC_FT_CACHE,0x02, struct sdc_shm_cache)

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif 

static inline int sdc_shm_cache_free(void* virt_addr, unsigned int size)
{
    unsigned int offset = ((unsigned long)virt_addr) & (PAGE_SIZE - 1);
    return munmap((char*)virt_addr - offset, size + offset);
}


#ifdef __cplusplus
}
#endif

#endif

