
#ifndef SDC_CODEC_IAAS_H
#define SDC_CODEC_IAAS_H

#include <stdint.h>
#include "sdc_video_iaas.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* codec.iaas.sdc 服务接口数据结构定义
*/

#define SDC_URL_ENCODED_JPEG	0x00

struct sdc_osd
{
	uint8_t format[128];
	uint32_t reserve;
	uint32_t content_len;
	uint8_t content[2048];
};

struct sdc_osd_region
{
	struct sdc_region region;
	struct sdc_osd osd;
};

struct sdc_encode_jpeg_param
{
	uint16_t qf;
	uint16_t osd_region_cnt;
	uint32_t reserve;
	struct sdc_region region;
	struct sdc_yuv_frame frame;
	struct sdc_osd_region osd_region[0];
};

struct sdc_jpeg_frame
{
	uint64_t addr_phy;
	uint64_t addr_virt;
	uint32_t size;
	uint32_t reserve;
	uint32_t cookie[4];
};

#define SDC_URL_DECODED_YUV	0x01

#define SDC_HEAD_DECODED_YUV_ACCEPT_TYPE	0x00

#define SDC_BGR888	0x10
#define SDC_RGB888	0x11


#define SDC_URL_OSD_BOX_HEIGHT	0x02

struct sdc_osd_box
{
	uint32_t width;
	struct sdc_osd osd;
};

#define SDC_URL_COMBINED_IMAGE	0x03
#define SDC_HEAD_COMBINED_CONTENT_TYPE	0x00
#define SDC_HEAD_COMBINED_JPEG_QF	0x01
#define SDC_HEAD_COMBINED_OSD	0x12 

struct sdc_combined_yuv
{
	struct sdc_region origin_region;
	struct sdc_region combined_region;
	struct sdc_yuv_frame frame;	
};

struct sdc_combined_yuv_param
{
	uint32_t width;
	uint32_t height;
	uint32_t yuv_cnt;
	uint32_t reserve;
	struct sdc_combined_yuv yuv[0];
};


#ifdef __cplusplus
}
#endif

#endif

