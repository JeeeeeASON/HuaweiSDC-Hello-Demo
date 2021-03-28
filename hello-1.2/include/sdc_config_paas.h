
#ifndef SDC_CONFIG_H
#define SDC_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SDC_URL_CONFIG_APP 1

/**
* 这是从配置服务发送到APP的请求，APP
*/
#define SDC_URL_CONFIG_PARAM 0

struct sdc_config_param {
    char* key;
    char* value;
};

struct sdc_config_param_list {
    uint32_t cnt;
    struct sdc_config_param* params;
};

#ifdef __cplusplus
}
#endif

#endif

