
#include "event.h"
#include "log.h"
#include "module.h"
#include "sdc.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "osd.h"
#include "meta.h"
#include "gateway.h"
#include "log.h"


static struct app_module g_module_end ;
static int app_event_run(struct app_ctx* app_ctx);
static void app_ctx_init(struct app_ctx* app_ctx, int epoll_fd);

/**
* app内部功能模块的初始化函数，都通过epoll驱动 
*/
static const pf_module_init g_modules[] = {
    module_gateway_init,
    module_config_init,
    module_osd_init,
    module_metadata_init,
    module_log_init,
};

int main(int argc, char* argv[])
{
    int i;
    int nret = 0;   //采用匈牙利命名法，n表示int，ret表示return，变量为一个用于作为返回值的int
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);    //创建一个epoll句柄
    struct app_ctx app_ctx = {};
    struct app_module** plast_module = 0;   //指向上一模块的指针（p表示pointer），初始化为0

    //如果epoll句柄创建失败，则跳至fail
    if (epoll_fd == -1) {
        LOGE("epoll_creat fail: %m");       //函数定义于log.c
        nret = errno;
        goto fail;
    }

    //app整体初始化，依赖于epoll
    app_ctx_init(&app_ctx, epoll_fd);
    plast_module = &app_ctx.module_list;    //将last_module指向app_ctx成员module_list。
 
    //依顺序对app各模块初始化
    for (i = 0; i < sizeof(g_modules) / sizeof(g_modules[0]); ++i) {
        *plast_module = g_modules[i](&app_ctx);
        if (!*plast_module) {
            LOGE("init g_modules[%d] fail: %m\n", i);   //任何模块初始化失败则记录下日志（函数定义于log.c），并跳转至fail处理。
            goto fail;
        }
        (*plast_module)->app = &app_ctx;    //main函数中生成的app_ctx地址写入module_list成员app。
        LOGI("module[%s] init success\n", (*plast_module)->name);
        plast_module = &(*plast_module)->next;  //last_module指向下一个模块入口。
    }
    *plast_module = &g_module_end;
    nret = app_event_run(&app_ctx); //启动app

//对于程序运行失败的处理，返回错误号
fail:
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
    return nret;
}

#define APP_EVENT_MAX 64

static int app_event_run(struct app_ctx* app_ctx)
{
    struct epoll_event events[APP_EVENT_MAX];
    struct app_event* app_event;
    int nret, i;

    /**************************************************************************************************
     * int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
     * ----------------------------------------------------------------------------------------------    
     *   等待事件的产生，类似于select()调用。参数events用来从内核得到事件的集合，maxevents告之内核这个events有多
     * 大，这个maxevents的值不能大于创建epoll_create()时的size，参数timeout是超时时间（毫秒，0会立即返回，-1将不
     * 确定，也有说法说是永久阻塞）。该函数返回需要处理的事件数目，如返回0表示已超时。如果返回–1，则表示出现错误，需要
     * 检查errno错误码判断错误类型。
     *   第1个参数epfd是epoll的描述符。
     *   第2个参数events则是分配好的epoll_event结构体数组，epoll将会把发生的事件复制到events数组中（events不
     * 可以是空指针，内核只负责把数据复制到这个 events数组中，不会去帮助我们在用户态中分配内存。内核这种做法效率很高）。
     *   第3个参数maxevents表示本次可以返回的最大事件数目，通常 maxevents参数与预分配的events数组的大小是相等的。
     *   第4个参数timeout表示在没有检测到事件发生时最多等待的时间（单位为毫秒），如果timeout为0，则表示epoll_wait在
     * rdllist链表中为空，立刻返回，不会等待。
     **************************************************************************************************/
    do {
        nret = epoll_wait(app_ctx->epoll_fd, events, sizeof(events) / sizeof(events[0]), -1);
        for (i = 0; i < nret; ++i) {
            app_event = (struct app_event*)events[i].data.ptr;
            app_event->handle(app_event, events[i].events, app_ctx);
        }
    }while (nret != -1 || errno != EINTR);
    return errno;
}

//app运行环境的初始化，参数为一个app_ctx结构体（app运行环境）的指针，和一个epoll句柄，将这个epoll句柄写在app的运行环境信息中
static void app_ctx_init(struct app_ctx* app_ctx, int epoll_fd)
{
    snprintf(app_ctx->name, sizeof(app_ctx->name), "%s", "hello sdc");
    app_ctx->epoll_fd = epoll_fd;   
}

//更新app下一模块的参数
int config_simple_update(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    return module->next->config_update(module->next, params, cnt);
}

int config_simple_get(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    return module->next->config_get(module->next, params, cnt);
}

void osd_simple_connect(struct app_module* module)
{
    module->next->osd_connect(module->next);
}

static int config_get_end(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    return 0;
}

static int config_update_end(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    return 0;
}

static void osd_connect_end(struct app_module* module)
{
}

static struct app_module g_module_end = {
    .config_get = config_get_end,
    .config_update = config_update_end,
    .osd_connect = osd_connect_end,
};
