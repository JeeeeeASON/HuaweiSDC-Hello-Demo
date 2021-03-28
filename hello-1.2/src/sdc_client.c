
#include "sdc_client.h"
#include "log.h"
#include "event.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/uio.h>
#include <errno.h>
#include <malloc.h>

static void sdc_client_try_connect(struct app_timer* app_timer, struct app_ctx* app_ctx);
static int sdc_client_reconnect(struct sdc_client* client, struct app_ctx* app_ctx);
static void sdc_client_on_input(struct app_event* app_event, uint32_t events, struct app_ctx* app_ctx);
static void sdc_client_on_connect(struct app_event* app_event, uint32_t events, struct app_ctx* app_ctx);

static const struct sdc_client_operations g_null_ops = {};

#define SDC_RECONN_DEFAULT 5000

/**
 * [sdc_client_init 对任意sdc_client进行初始化。]
 * @param client          [将要初始化的sdc_client]
 * @param server_name     [需要连接的服务包路径]
 * @param reconn_interval [连接失败时，尝试重连的时间间隔，如果为0则会被置为默认的重连间隔SDC_RECONN_DEFAULT]
 * @param sdc_ops         [该客户端可能的操作集，如果为null则会被指向一个空的操作集]
 */
void sdc_client_init(struct sdc_client* client, const char* server_name, int reconn_interval, const struct sdc_client_operations* sdc_ops)
{
    app_timer_init(&client->reconn_timer, sdc_client_try_connect);  //初始化一个计时器。
    app_event_init(&client->sdc_event, -1, 0);  //初始化本客户端所对应的事件。
    
    client->server_name = server_name;
    client->reconn_interval = reconn_interval ? reconn_interval : SDC_RECONN_DEFAULT;
    client->sdc_ops = sdc_ops ? sdc_ops : &g_null_ops;
    client->connected = 0;
}

/**
 * [sdc_client_connect 打开客户端对应的服务器文件，若失败则不断尝试打开。同时记录日志。对客户端代表的事件进行初始化并将事件添加进epoll列表。]
 * @param  client  [记载事件的客户端]
 * @param  app_ctx [APP运行环境]
 * @return         [app_event_add()的返回值，添加事件的返回号，标志连接失败或者添加epoll事件成功与否。]
 */
int sdc_client_connect(struct sdc_client* client, struct app_ctx* app_ctx)
{
    int nret;
    int fd = open(client->server_name, O_RDWR | O_NONBLOCK | O_CLOEXEC);

    if (fd == -1) {
        LOGE("connect %s fail, %m", client->server_name);
        if (client->reconn_interval) {  //若reconn_interval大于0，则持续尝试重连。
            return sdc_client_reconnect(client, app_ctx);
        }else {
            return -1;  
        }
    }
    LOGI("connect %s success", client->server_name);

    app_event_init(&client->sdc_event, fd, sdc_client_on_connect);
    nret = app_event_add(&client->sdc_event, EPOLLOUT, app_ctx);
    if (nret) {
        LOGE("connected %s, fd: %d, epoll_fd: %d, app_event_add fail: %d, %m", client->server_name, fd, app_ctx->epoll_fd, nret);
        /** 这是本地出错了，不再重连，直接返回错误 */
        app_event_init(&client->sdc_event, -1, 0);
        close(fd);
    }
    return nret;
}

/**
 * [sdc_client_close 关闭对应的客户端，某件事件处理完成。]
 * @param client  [需要被关闭的客户端]
 * @param app_ctx [APP运行环境]
 */
void sdc_client_close(struct sdc_client* client, struct app_ctx* app_ctx)
{
    LOGI("client %s close", client->server_name);   //输出日志：关闭某一客户端。
    if (client->connected) {    //如果当前客户端connected标志非0，则执行关闭处理，将该标志置零，否则不执行任何操作。
        client->connected = 0;
        if (client->sdc_ops->close) {   //如果当前客户端的操作集中包含close操作，则执行close，否则跳过。
            client->sdc_ops->close(client, app_ctx);
        }
    }
    app_event_del_close(&client->sdc_event, app_ctx);   //关闭客户端代表的事件。

    if (client->reconn_interval) {  //若客户端的重连间隔不为0，则表示代码未进入上一段if，客户端处于重连阶段，正常执行重连操作
        (void)sdc_client_reconnect(client, app_ctx);
    }else {
        app_timer_del(&client->reconn_timer, app_ctx);  //若客户端并非处于重连状态，则正常关闭最后一项服务：计时器。
    }
}


/**
 * [sdc_client_reconnect 客户端重连函数，在客户端尝试连接失败时调用，主要修改了客户端的重连间隔与计时器。]
 * @param  client  [执行重连的客户端]
 * @param  app_ctx [APP运行环境]
 * @return         [返回计时器添加的结果，或者0值]
 */
static int sdc_client_reconnect(struct sdc_client* client, struct app_ctx* app_ctx)
{
    app_timer_del(&client->reconn_timer, app_ctx);  //清除上一次重连的计时器。
    if (client->reconn_interval) {  //若客户端的重连间隔非0，即表示客户端尝试重连，则添加一个重连计时器。
        return app_timer_add(&client->reconn_timer, client->reconn_interval, client->reconn_interval, app_ctx);
    }
    return 0;
}

/**
 * [sdc_client_try_connect 整个程序中没有调用，看起来像是华为开发人员用于测试客户端连接功能的假人函数]
 * @param app_timer []
 * @param app_ctx   [description]
 */
static void sdc_client_try_connect(struct app_timer* app_timer, struct app_ctx* app_ctx)
{
    uint64_t val;
    struct sdc_client* client = container_of(app_timer, struct sdc_client, reconn_timer);

    int fd = open(client->server_name, O_RDWR | O_NONBLOCK | O_CLOEXEC);

    (void)read(app_timer->base.fd, &val, sizeof(val));
    if (fd == -1) {
        LOGE("try connect %s fail, %m", client->server_name);
        return;
    }
    LOGI("try connect %s success", client->server_name);

    app_event_init(&client->sdc_event, fd, sdc_client_on_connect);
    int nret = app_event_add(&client->sdc_event, EPOLLOUT, app_ctx);

    if (nret) {
        /** 本地资源出错，在自动重连环节，继续尝试，记录日志 */
        LOGE("app_event_add fail: %s", client->server_name);
        app_event_init(&client->sdc_event, -1, 0);
        close(fd);
    }else {
        app_timer_del(app_timer, app_ctx);
    }
}

static void sdc_client_on_connect(struct app_event* app_event, uint32_t events, struct app_ctx* app_ctx)
{
    struct sdc_client* client = container_of(app_event, struct sdc_client, sdc_event);
    int nret;

    LOGI("%s  connect ...", client->server_name); 
    if (client->sdc_ops && client->sdc_ops->open) {
        nret = client->sdc_ops->open(client, app_ctx);
        if (nret) {
            app_event_del_close(app_event, app_ctx);
            (void)sdc_client_reconnect(client, app_ctx);
            LOGW("%s  connect fail", client->server_name); 
            return;
        }
    }
    app_event->handle = sdc_client_on_input;
    app_event_mod(app_event, EPOLLIN, app_ctx);
    client->connected = 1;
}

static void sdc_client_on_input(struct app_event* app_event, uint32_t events, struct app_ctx* app_ctx)
{
    struct sdc_client* client = container_of(app_event, struct sdc_client, sdc_event);
    uint32_t len;
    char* buf = 0;
    struct sdc_common_head* head = 0;

    int nret = ioctl(app_event->fd, SDC_BUFF_GETMSGSIZE, &len);

    if (nret == -1) {
        LOGE("%s  ioctl fail, %m", client->server_name); 
        goto fail;
    }

    buf = malloc(len);
    if (!buf) {
        errno = ENOMEM;
        goto fail;
    }

    ssize_t ret = read(app_event->fd, buf, len);
    if (ret == -1) {
        LOGE("%s  read fail, %m", client->server_name); 
        goto fail;
    }

    head = (struct sdc_common_head*)buf;

    LOGD("%s  read %zd, response: %d", client->server_name, ret, head->response); 
    if (head->response) {
        if (client->sdc_ops->handle_response) {
            client->sdc_ops->handle_response(client, head, app_ctx);
        }else {
            LOGI("discard response from: %s, code=%d", client->server_name, head->code);
        }
    }else {
        if (client->sdc_ops->handle_request) {
            client->sdc_ops->handle_request(client, head, app_ctx);
        }else {
            LOGI("discard request from: %s", client->server_name);
        }
    }
    free(buf);
    return;
fail:
    if (buf) {
        free(buf);
    }
    if (client->sdc_ops->handle_error) {
        client->sdc_ops->handle_error(client, errno, app_ctx);
    }
    return;
}
