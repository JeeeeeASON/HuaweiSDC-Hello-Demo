
#ifndef SDC_UTILS_H
#define SDC_UTILS_H

#include <stdint.h>
#include <sys/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* utils.iaas.sdc服务接口数据结构定义
*/
#define SDC_URL_HARDWARE_ID	100

struct sdc_hardware_id
{
	char id[33];
};

#define SDC_URL_MMZ	101

#define SDC_HEAD_MMZ_CACHED	0x00

struct sdc_mmz
{
	uint64_t addr_phy;
	uint64_t addr_virt;
	uint32_t size;
	uint32_t reserve;
	uint32_t cookie[4];
};

#ifdef __cplusplus
}
#endif

#endif

