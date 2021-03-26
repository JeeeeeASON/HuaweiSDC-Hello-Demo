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
 * sdc_ops：一个常量结构体，其成员是一系列函数指针，代表所有作为一个sdc_client可能采取的操作。
 * server_name：作为服务器被打开的服务文件（如video.iaas.sdc）路径。
 * reconn_interval：连接服务器失败时尝试重新连接的间隔。
 * connected：连接成功的指示变量。
 */
struct sdc_client {
    struct app_event sdc_event;
    struct app_timer reconn_timer;
    const struct sdc_client_operations* sdc_ops;
    const char* server_name;
    int reconn_interval;
    unsigned int connected: 1;
};

/**
 * [sdc_client_fd 这是一个静态内联函数，返回该客户所承载的sdc_event的文件描述符]
 * @param  client [想要获知文件描述符的客户端的指针]
 * @return        [客户端的文件描述符]
 * 
 * 1.   static修饰符表示该函数仅在sdc_client.h中可见，其他文件不能引用这个函数，同时也意味着其他文件也可以声明、定义同名的函数。
 * 
 * 2.   inline修饰符表示该函数是一个内联函数，在调用时不需要像普通函数一样压栈，而是将函数语句直接填充到函数调用的位置。一般用于
 * 重复多次调用但是函数体简单的函数。
 *
 * 3.   static inline在头文件中相当于一个固定搭配。因为这个函数经常被调用，而开发者又不想在每次调用的时候都进行一次压栈出栈，所
 * 以会选择使用内联inline进行优化。为了使多个文件都能调用这个函数，就必须在头文件中进行声明，然后让其他C文件包含头文件。但同时如
 * 果在头文件中声明，.c文件中使用内联，根据gcc编译器的编译原则，以.c文件为单位编译，当.c文件调用外部函数时，会预留一个符号记录这
 * 些函数，直到所有obj编译生成后才会给这些符号函数的地址。所以对于普通地在.c文件中内联的函数，其他文件编译时只能见到这个函数的声
 * 明，于是会像普通函数一样记录一个地址以进入函数，达不到内联的效果，因此必须要在头文件中声明并实现内联函数。
 *      而在头文件中声明并实现的内联函数会遇到一个问题：inline修饰符是“建议编译器内联”而不是“强制”，所以在编译器的优化环节，很有
 * 可能会忽略内联修饰符。加上static修饰符后，编译器优化时实现内联的可能性大大增加。
 */
static inline int sdc_client_fd(const struct sdc_client* client)
{
    return client->sdc_event.fd;
}

/**
 * 记录每一个sdc_client的可能操作的操作集结构体，其成员分别是一系列函数指针：
 * *open：打开
 * *close：关闭
 * *handle_request：发送请求的句柄。
 * *handle_response：响应请求的句柄。
 * *handle_error：处理错误的句柄。
 */
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

