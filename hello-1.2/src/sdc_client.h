/* 
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: 
 * Author: h00339793
 * Create: 2020-07-13
 * Notes: 
 */
#ifndef SDC_CLIENT_H
#define SDC_CLIENT_H

#include "event.h"
#include "sdc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sdc_common_head; //APP作为客户端与服务层的.sdc文件作为服务器传递消息报时所用的消息头。
struct sdc_client_operations;   //APP的操作

/**
 * 代表APP作为客户端的结构体，其成员列表如下：
 * sdc_event：一个app_event结构体，用来将APP行为翻译为客户端行为。
 * reconn_timer：用于在客户端重连时计时。
 * sdc_ops：
 */
struct sdc_client {
    struct app_event sdc_event;
    struct app_timer reconn_timer;
    const struct sdc_client_operations* sdc_ops;
    const char* server_name;
    int reconn_interval;
    unsigned int connected: 1;
};

static inline int sdc_client_fd(const struct sdc_client* client)
{
    return client->sdc_event.fd;
}

struct sdc_client_operations {
    int (*open)(struct sdc_client* client, struct app_ctx* app_ctx);
    void (*close)(struct sdc_client* client, struct app_ctx* app_ctx);
    void (*handle_request)(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx);
    void (*handle_response)(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx);
    void (*handle_error)(struct sdc_client* client, int errno, struct app_ctx* app_ctx);
};

void sdc_client_init(struct sdc_client* client, const char* server_name, int reconn_interval, const struct sdc_client_operations* sdc_ops);

int sdc_client_connect(struct sdc_client* client, struct app_ctx* app_ctx);

void sdc_client_close(struct sdc_client* client, struct app_ctx* app_ctx);

#ifdef __cplusplus
}
#endif

#endif

