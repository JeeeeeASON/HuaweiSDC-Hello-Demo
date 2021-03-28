
#ifndef SDC_VIDEO_IAAS_H
#define SDC_VIDEO_IAAS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* video.iaas.sdc服务的数据结构定义
*/

/**
* YUV Channel的数据格式定义
*/

#define SDC_URL_YUV_CHANNEL 	0x00

#define SDC_YVU_420SP 		0x00

struct sdc_yuv_channel_param
{
	uint32_t channel;
	uint32_t width;
	uint32_t height;
	uint32_t fps;
	uint32_t on_off;
	uint32_t format; // YUV_420SP 当前发的临时版本没有这个字段，正式版本补充进去
};

struct sdc_resolution {
	uint32_t width;
	uint32_t height;
};

struct sdc_yuv_channel_info
{
	struct sdc_yuv_channel_param param;
	struct sdc_resolution max_resolution;
	uint32_t is_snap_channel;
	uint32_t src_id;
	uint32_t subscribe_cnt;
    uint32_t resolution_modtify;
};

#define SDC_URL_YUV_DATA		0x01

#define SDC_HEAD_YUV_SYNC 		0x00
#define SDC_HEAD_YUV_CACHED_COUNT_MAX 	0x01
#define SDC_HEAD_YUV_PARAM_MASK 	0x02

#define SDC_HEAD_YUV_CACHED_COUNT 	0x10
#define SDC_HEAD_YUV_PARAM_SNAP 	0x13


struct sdc_yuv_frame
{
	uint64_t addr_phy;
	uint64_t addr_virt;
    uint32_t size;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t format; // YUV_420SP
	uint32_t reserve;
	uint32_t cookie[4];
};

struct sdc_yuv_data
{
	uint32_t channel;
	uint32_t reserve;
	uint64_t pts;
	uint64_t pts_sys;
	struct sdc_yuv_frame frame;
};

struct sdc_chan_query
{
	uint32_t channel;
};

#define SDC_URL_VENC_DATA	0x03

#define SDC_HEAD_VENC_CACHED_COUNT_MAX	0x01
#define SDC_HEAD_VENC_CACHED_COUNT	0x10

// #define SDC_VENC_FRAME_I	0x00
// #define SDC_VENC_FRAME_P	0x01
// #define SDC_VENC_FRAME_B	0x02

struct sdc_venc_frame
{
	uint64_t addr_phy;
	uint64_t addr_virt;
	uint64_t size;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t frame_type;
	uint64_t cookie[8];
};

struct sdc_venc_data
{
	uint32_t channel;
	uint32_t reserve;
	uint64_t ulpts;
	uint64_t pts_sys;
	struct sdc_venc_frame frame;
};

#define SDC_URL_YUV_SNAP		0x04

struct sdc_yuv_snap_param
{
	uint32_t id;
	uint32_t num;
	uint32_t interval_msec;
};

#define SDC_URL_RED_LIGHT_ENHANCED	0x05

struct sdc_region
{
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

struct sdc_red_light_enhanced_param
{
	uint32_t level;
	uint32_t num;
	uint32_t uImgWidth;  /*图像的宽度，用来计算比例*/
    uint32_t uImgHeight;  /*图像的高度，用来计算比例*/
	struct sdc_region regions[0];
};

#ifdef __cplusplus
}
#endif

#endif

