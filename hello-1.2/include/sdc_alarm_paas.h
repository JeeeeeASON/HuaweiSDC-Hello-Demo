
#ifndef SDC_ALARM_H
#define SDC_ALARM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* 告警源注册，消息体为告警源定义的文件名称
*/
#define SDC_URL_ALARM_SOURCE	1

#define SDC_URL_ALARM   3

struct sdc_alarm {
    char name[64];
    char source[32];
    char data[0];
};

#ifdef __cplusplus
}
#endif

#endif

