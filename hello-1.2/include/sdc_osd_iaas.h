
#ifndef SDC_OSD_H
#define SDC_OSD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* 告警源注册，消息体为告警源定义的文件名称
*/
#define SDC_URL_OSD_ENV_NAME	21

struct sdc_osd_env_name {
    char name[128];
};

#define SDC_URL_OSD_ENV_DESC    24

#define SDC_OSD_LANG_ZH 0x5A48
#define SDC_OSD_LANG_EN 0x454E
#define SDC_OSD_LANG_ES 0x4553
#define SDC_OSD_LANG_FR 0x4652

struct sdc_osd_env_desc {
    uint32_t lang;
    char name[128];
    char desc[128];
};

#define SDC_URL_OSD_ENV_VALUE   23
struct sdc_osd_env_value {
    char name[128];
    char value[128];
};

#ifdef __cplusplus
}
#endif

#endif

