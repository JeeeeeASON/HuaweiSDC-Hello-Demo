
#ifndef SDC_EVENT_PAAS_H
#define SDC_EVENT_PAAS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 
* event.paas.sdc服务接口数据结果定义 
*/
#define event_paas_name "/mnt/srvfs/event.paas.sdc"

#define SDC_URL_PAAS_EVENTD_EVENT 0
#define SDC_HEAD_SHM_CACHED_EVENT 0xFFFF

struct paas_event
{
        char publisher[16];   //发送事件的服务标识，调测使用
        char name[16];      //事件唯一标识，建议同域名定义避免冲突
        uint64_t src_timestamp;  //发生时的时间，单位毫秒（CLOCK_MONOTONIC时间）
        uint64_t tran_timestamp; //服务转发的时间，单位毫秒（CLOCK_MONOTONIC时间）
        uint32_t id;        //建议同IP地址一样管理，不同前缀对应事件分类，方便分类订阅。
        uint32_t length;   //事件内容的长度.
        char data[0];
};

struct paas_shm_cached_event
{
	uint64_t addr_phy;
	uint32_t size;
	uint32_t cookie;
};

struct paas_event_filter
{
        char subscriber[16]; //订阅者的标识, 调测使用
        char name[16];
        char filter[256];
};

#define SDC_URL_PAAS_EVENTD_SUBSCRIBE_STAT 0X01

struct paas_event_subscribe_stat
{
        char name[16];
        uint64_t cnt;
        uint64_t fail_cnt;
};

#define SDC_URL_PAAS_EVENTD_PUBLISH_STAT 0X02

struct paas_event_publish_stat
{
        char name[16];
        uint32_t subscriber_cnt;
        uint64_t cnt;
        uint64_t trans_cnt; //成功转发的次数
        uint64_t fail_cnt; //转发失败的次数
};

#ifdef __cplusplus
}
#endif

#endif

