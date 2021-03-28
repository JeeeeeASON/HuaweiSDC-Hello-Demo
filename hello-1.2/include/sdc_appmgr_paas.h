
#ifndef SDC_APPMGR_PAAS_H
#define SDC_APPMGR_PAAS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* appmgr.paas.sdc服务接口数据结构定义
*/

#define SDC_URL_APP_WATCHDOG 200

struct appdog_op_req{
    int32_t watchdog_time;
};

#ifdef __cplusplus
}
#endif

#endif

