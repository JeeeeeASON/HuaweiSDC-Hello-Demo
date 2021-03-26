/**
 * 包含所有APP模块相关信息的声明，例如APP运行环境app_ctx，APP模块的注册信息app_module
 */
#ifndef APP_MODULE_H
#define APP_MODULE_H

#include "sdc_client.h"

#ifdef __cplusplus
extern "C" {
#endif

struct app_module;
struct sdc_config_param;

#define APP_NAME_MAX 32
struct app_ctx {
    int epoll_fd;   //记录epoll文件的描述符
    char name[APP_NAME_MAX];    //存储APP名称
    struct app_module* module_list; //记录APP第一个模块的地址，作为一整个APP模块链表的表头。
};

typedef struct app_module* (*pf_module_init)(struct app_ctx* ctx);  //声明一个

/**
* 以下指针都不能为空，如果本模块没有对应实现，可以设置为__xxx__simple__xxx(...)，参见本头文件最后的声明.
*/
struct app_module {
    /** 
    * 始终返回本模块的配置项数量+next的配置项，即便list中的空间不足，用于app计算总配置项
    */
    int (*config_get)(struct app_module* module, struct sdc_config_param* params, int cnt);
    /**
    * 更新本模块的配置项数量，list中可能有非本模块的，需要各个模块自己过滤
    */
    int (*config_update)(struct app_module* module, struct sdc_config_param* params, int cnt);

    /**
    * 通知各个模块osd模块可用了，各个模块可以注册自己的OSD变量
    */
    void (*osd_connect)(struct app_module* module);

    /**
    * 上述接口在本模块处理完毕之后都应该交给下个模块处理，这是约定
    */
    struct app_module* next;
    struct app_ctx* app;
    const char* name;
};

int config_simple_get(struct app_module* module, struct sdc_config_param* params, int cnt);

int config_simple_update(struct app_module* module, struct sdc_config_param* params, int cnt);

void osd_simple_connect(struct app_module* module);

/**
 * [APP_NULL_MODULE APP的默认模块内容，用于某一个app_module中函数指针为空时，将其置为默认值，如n.config_get置为默认的config_simple_get]
 * @param  n [模块名，或者说app_module的变量名]
 * @return   [无返回值]
 */
#define APP_NULL_MODULE(n) \
    { \
        .config_get = config_simple_get, \
        .config_update = config_simple_update, \
        .osd_connect = osd_simple_connect, \
        .name = (n), \
    }

#ifdef __cplusplus
}
#endif

#endif

