
#include "event.h"
#include "log.h"
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

/**
 * [app_event_init 对app的某一事件进行初始化]
 * @param app_event [指向某一事件结构体的指针]
 * @param fd        [内核在创建sdc_client时打开服务包文件（如"/mnt/srvfs/event.paas.sdc"）时给分配的文件描述符]
 * @param handle    [该事件对应的句柄]
 */
void app_event_init(struct app_event* app_event, int fd, pf_app_event_handle handle)
{
    app_event->fd = fd;
    app_event->handle = handle;
}

/**
 * [app_event_add 添加一个app事件，实际是将新的app_event包装成为epoll事件，交由epoll处理。以下的三个函数功能都类似。]
 * @param  event   [被添加的事件指针]
 * @param  events  [向epoll注册的事件的类型]
 * @param  app_ctx [app运行环境]
 * @return         [返回epoll对于事件列表进行修改的结果]
 */
int app_event_add(struct app_event* event, uint32_t events, struct app_ctx* app_ctx)
{
    struct epoll_event epoll_event;

    epoll_event.events = events;
    epoll_event.data.ptr = event;
    return epoll_ctl(app_ctx->epoll_fd, EPOLL_CTL_ADD, event->fd, &epoll_event);
}

int app_event_mod(struct app_event* event, uint32_t events, struct app_ctx* app_ctx)
{
    struct epoll_event epoll_event;

    epoll_event.events = events;
    epoll_event.data.ptr = event;
    return epoll_ctl(app_ctx->epoll_fd, EPOLL_CTL_MOD, event->fd, &epoll_event);
}

void app_event_del(struct app_event* event, struct app_ctx* app_ctx)
{
    struct epoll_event epoll_event;
    (void)epoll_ctl(app_ctx->epoll_fd, EPOLL_CTL_DEL, event->fd, &epoll_event);
}

void app_event_del_close(struct app_event* event, struct app_ctx* app_ctx)
{
    app_event_del(event, app_ctx);
    close(event->fd);
    event->fd = -1;
}

/**
 * [app_timer_handle app计时器的句柄]
 * @param app_event [description]
 * @param events    [description]
 * @param app_ctx   [description]
 */
static void app_timer_handle(struct app_event* app_event, uint32_t events, struct app_ctx* app_ctx);

void app_timer_init(struct app_timer* timer, pf_app_timer_handle handle)
{
    app_event_init(&timer->base, -1, app_timer_handle);
    timer->handle = handle;
    timer->interval = 0;
}

#define TIME_UNIT 1000

int app_timer_add(struct app_timer* timer, uint32_t timeout, uint32_t interval, struct app_ctx* app_ctx)
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    
    if (fd == -1) {
        goto fail;
    }

    struct itimerspec itimerspec;

    itimerspec.it_value.tv_sec = timeout / TIME_UNIT;
    itimerspec.it_value.tv_nsec = (timeout % TIME_UNIT) * TIME_UNIT * TIME_UNIT;
    itimerspec.it_interval.tv_sec = interval / TIME_UNIT;
    itimerspec.it_interval.tv_nsec = (interval % TIME_UNIT) * TIME_UNIT * TIME_UNIT;

    LOGD("timeout: %u(ms), iterval %d(ms), value %ld,%ld, interval: %ld,%ld\n", timeout, interval,
        itimerspec.it_value.tv_sec, itimerspec.it_value.tv_nsec,
        itimerspec.it_interval.tv_sec, itimerspec.it_interval.tv_nsec);

    int nret = timerfd_settime(fd, 0, &itimerspec, 0);

    if (nret == -1) {
        goto fail;
    }

    if (timer->base.fd >= 0) {
        app_timer_del(timer, app_ctx);
    }
    
    timer->base.fd = fd;
    timer->interval = interval;

    nret = app_event_add(&timer->base, EPOLLIN, app_ctx);

    if (nret == -1) {
        goto fail;
    }
    return 0;
fail:
    if (fd >= 0) {
        close(fd);
    }
    return errno;
}

void app_timer_del(struct app_timer* timer, struct app_ctx* app_ctx)
{
    app_event_del_close(&timer->base, app_ctx);
}


static void app_timer_handle(struct app_event* app_event, uint32_t events, struct app_ctx* app_ctx)
{
    struct app_timer* app_timer = container_of(app_event, struct app_timer, base);
    app_timer->handle(app_timer, app_ctx);
    if (app_timer->interval == 0) {
        app_timer_del(app_timer, app_ctx);
    }
}

