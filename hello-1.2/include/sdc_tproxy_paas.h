#ifndef SDC_TPROXY_PAAS_H
#define SDC_TPROXY_PAAS_H

#include "sdc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PAAS_TPROXY_SERVER "tproxy.paas.sdc"

#define SDC_URL_TPROXY_SERVER 0

/**
* domain: 当前仅支持AF_UNIX
* type: SOCK_STREAM | SOCK_DGRAM
* addr: unixdomain socket文件路径
*/
struct sdc_tproxy_server 
{
    int domain;
    int type;
    char addr[128];
    char filter[0];
};

#define SDC_URL_TPROXY_CONNECTION 1

/**
* 提供TCP/UDP正向代理功能，创建成功之后，服务句柄将可以直接和外部实现网络通信，读写协议和SDC服务通信协议无关
* domain: 当前仅支持AF_INET
* type：SOCK_STREAM | SOCK_DGRAM
* addr: <ip>:<port>这种格式的字符串
*/
struct sdc_tproxy_connection
{
    int domain;
    int type;
    char addr[128];
};


#ifdef __cplusplus
}
#endif

#endif

