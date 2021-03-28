
#ifndef SDC_COMMON_H
#define SDC_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* 消息头的数据结构定义
*/
#define SDC_VERSION 0x5331

#define SDC_METHOD_CREATE 1
#define SDC_METHOD_GET 2
#define SDC_METHOD_UPDATE 3
#define SDC_METHOD_DELETE 4

#define SDC_CODE_200	200
#define SDC_CODE_400	400
#define SDC_CODE_401	401
#define SDC_CODE_403	403
#define SDC_CODE_500	500

struct sdc_common_head
{
        uint16_t        version;
        uint8_t         url_ver;
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        uint8_t         method: 7;
        uint8_t         response: 1;
#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
        uint8_t         response: 1;
        uint8_t         method: 7;
#else
#error "unknown __BYTE_ORDER__"
#endif
#else
#error "don't define __BYTE_ORDER__ or __ORDER_LITTLE_ENDIAN__ or __ORDER_BIG_ENDIAN__"
#endif
        uint16_t        url;
        uint16_t        code;
        uint16_t        head_length;
        uint16_t        trans_id;
        uint32_t        content_length;
};


struct sdc_extend_head
{
        uint16_t type;
        uint16_t length;
        uint32_t reserve;
};

#define sdc_extend_head_length(extend_head) (((extend_head)->length + 7) & ~7)

#define sdc_extend_head_next(extend_head) ((struct sdc_extend_head*)((char*)extend_head + sdc_extend_head_length(extend_head)))

#define sdc_extend_head_first(common_head) ((struct sdc_extend_head*)(common_head + 1))

#define sdc_for_each_extend_head(common_head, extend_head) \
	for( extend_head = sdc_extend_head_first(common_head); (char*)extend_head - (char*)common_head < common_head->head_length; extend_head = sdc_extend_head_next(extend_head))

/**
* 通用扩展头的定义
*/

/**
* 定义服务消息的内容是否利用了零拷贝机制进行传递
* 如果是内容就应该是通过sdc_shm.h中定义的数据结构,传递的是共享内存的地址信息
* 一般而言是指HBTP的消息体是共享内存，但不同接口可能有些差别
* 比如告警接口中，指的是告警头之外的内容是共享内存传递，需要参考具体服务接口的详细定义
*/
#define SDC_HEAD_TRANSFER_ENCODING 0x8000
#define SDC_HEAD_TRANSFER_COPY 0x00
#define SDC_HEAD_TRANSFER_ZCOPY 0x01

/**
* 同一个服务接口可能传递不同的数据结构，需要在扩展头中指定具体的数据结构类型
* 类型值的定义由各个服务接口自己决定，即同一个取值在不同服务接口之间可能代表不同含义
*/
#define SDC_HEAD_CONTENT_TYPE 0x8001

#define SDC_HEAD_HOST 0x8002
#define SDC_HEAD_USER_AGENT 0x0x8003

/** 
* 通用的数据结构定义
* 服务接口支持传递指针数据，为了兼顾32/64位平台，要求指针字段必须是8字节
*/

typedef union _uint64ptr
{
    void* pvoid;
    char* pchar;
    uint64_t u64;
    uint32_t u32;
} uint64ptr_t;

#ifdef __cplusplus
}
#endif

#endif

