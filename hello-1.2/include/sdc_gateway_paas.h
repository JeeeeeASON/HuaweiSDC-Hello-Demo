
#ifndef SDC_GATEWAY_PAAS_H
#define SDC_GATEWAY_PAAS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* gateway.paas.sdc服务接口数据结构定义
*/

#define SDC_URL_PAAS_GATEWAY_CONF 00

struct paas_gateway_conf {
    char path[256];
};

#ifdef __cplusplus
}
#endif

#endif

