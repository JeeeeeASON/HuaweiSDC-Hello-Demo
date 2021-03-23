
#include "config.h"
#include "log.h"
#include "sdc_client.h"
#include "sdc.h"
#include "module.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>

struct module_config_ctx {
    struct sdc_client base;
    struct app_module module;
};

static struct module_config_ctx g_config_ctx = {
    .module = APP_NULL_MODULE("config"),
};


static int config_open(struct sdc_client* client, struct app_ctx* app_ctx);
static void config_close(struct sdc_client* client, struct app_ctx* app_ctx);
static void config_handle_request(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx);
static void config_handle_response(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx);
static void config_handle_error(struct sdc_client* client, int err, struct app_ctx* app_ctx);

static const struct sdc_client_operations g_config_ops = {
    .open = config_open,
    .close = config_close,
    .handle_request = config_handle_request,
    .handle_response = config_handle_response,
    .handle_error = config_handle_error,
};

struct app_module* module_config_init(struct app_ctx* app_ctx)
{
    sdc_client_init(&g_config_ctx.base, "/mnt/srvfs/config.paas.sdc", 0, &g_config_ops);
    return sdc_client_connect(&g_config_ctx.base, app_ctx) ? 0 : &g_config_ctx.module;
}

static int config_open(struct sdc_client* client, struct app_ctx* app_ctx)
{
    const char* usr_config = "./config.conf";
    struct sdc_common_head head = {
        .version = SDC_VERSION,
        .url = SDC_URL_CONFIG_APP,
        .method = SDC_METHOD_CREATE,
        .head_length = sizeof(head),
        .content_length = strlen(usr_config) + 1,
    };

    struct iovec iov[2];

    iov[0].iov_base = &head;
    iov[0].iov_len = sizeof(head);
    iov[1].iov_base = (void*)usr_config;
    iov[1].iov_len = head.content_length;

    ssize_t ret = writev(sdc_client_fd(client), iov, sizeof(iov) / sizeof(iov[0]));
    LOGI("register to config: %s ...: ret: %zd", usr_config, ret);

    if (ret == -1) {
        /** 出错了，可能是服务端故障, 关闭后会自动重连 */
        sdc_client_close(client, app_ctx);
        LOGI("register to config fail: %m");
        return errno;
    }

    return 0;
}

static void config_close(struct sdc_client* client, struct app_ctx* app_ctx)
{
    LOGE("config connection closed!");
}

static void config_handle_error(struct sdc_client* client, int err, struct app_ctx* app_ctx)
{
    LOGE("config handle error: %d", err);
    sdc_client_close(client, app_ctx);
}


static void config_handle_request(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx);
static void config_handle_response(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx)
{
    if (head->code != SDC_CODE_200) {
        LOGW("config handle response: errcode of response: %d", head->code);
        sdc_client_close(client, app_ctx);
    }
}

static void config_get_param(struct module_config_ctx* config_ctx, struct sdc_common_head* head, struct app_ctx* app_ctx);
static void config_update_param(struct module_config_ctx* config_ctx, struct sdc_common_head* head, struct app_ctx* app_ctx);

static void config_handle_request(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx)
{
    struct module_config_ctx* config_ctx = container_of(client, struct module_config_ctx, base);

    if (head->url != SDC_URL_CONFIG_PARAM) {
        return ;
    }
    switch (head->method) {
        case SDC_METHOD_GET:
            config_get_param(config_ctx, head, app_ctx);
            break;
        case SDC_METHOD_UPDATE:
            config_update_param(config_ctx, head, app_ctx);
            break;
        default:
            break;
    };
}

static void config_get_param(struct module_config_ctx* config_ctx, struct sdc_common_head* head, struct app_ctx* app_ctx)
{
    struct iovec iov[2];
    struct sdc_config_param_list param_list = {};
    ssize_t ret;

    LOGD("+++ param_list.cnt: %d", param_list.cnt);
    param_list.cnt = app_ctx->module_list->config_get(app_ctx->module_list, 0, 0);
    LOGD("--- param_list.cnt: %d", param_list.cnt);

    param_list.params = malloc(param_list.cnt * sizeof(*param_list.params));
    if (!param_list.params) {
        head->code = SDC_CODE_500;
        head->content_length = 0;
        head->head_length = sizeof(*head);
        LOGE("get param response: nomemory ");
        ret = write(sdc_client_fd(&config_ctx->base), head, sizeof(*head));
        goto fail;
    }
    (void)app_ctx->module_list->config_get(app_ctx->module_list, param_list.params, param_list.cnt);

    for (int i = 0; i < param_list.cnt; ++i) {
        LOGD("param: key= %s, value=[%s]\n", param_list.params[i].key, param_list.params[i].value);
    }

    /** 重复利用请求的head数据 */
    iov[0].iov_base = head;
    iov[0].iov_len = sizeof(*head);
    iov[1].iov_base = &param_list;
    iov[1].iov_len = sizeof(param_list);

    head->response = 1;
    head->code = SDC_CODE_200;
    /** 忽略所有的扩展头 */
    head->head_length = sizeof(*head);
    head->content_length = sizeof(param_list);

    LOGI("get param response ");
    ret = writev(sdc_client_fd(&config_ctx->base), iov, sizeof(iov) / sizeof(iov[0]));
    free(param_list.params);
fail:
    if (ret == -1) {
        LOGE("get param response fail: %m");
        /** 出错之后就断开重连 */
        sdc_client_close(&config_ctx->base, app_ctx);
    }
}

static void config_update_param(struct module_config_ctx* config_ctx, struct sdc_common_head* head, struct app_ctx* app_ctx)
{
    struct sdc_config_param_list* param_list ;
    ssize_t ret;

    if (head->content_length != sizeof(*param_list)) {
        head->response = 1;
        head->code = SDC_CODE_400;
        head->head_length = sizeof(*head);
        head->content_length = 0;
        LOGE("update param request wrong");
        goto out;
    }

    param_list = (struct sdc_config_param_list*)((char*)head + head->head_length);
   
    ret = app_ctx->module_list->config_update(app_ctx->module_list, param_list->params, param_list->cnt);

    /** 复用请求的head，回送响应 */
    head->code = ret ? SDC_CODE_400 : SDC_CODE_200;
    head->response = 1;
    head->head_length = sizeof(*head);
    head->content_length = 0;
    LOGE("update param return...");
out:
    ret = write(sdc_client_fd(&config_ctx->base), head, sizeof(*head));
    if (ret == -1) {
        LOGE("update param response fail: %m");
        /** 出错之后就断开重连 */
        sdc_client_close(&config_ctx->base, app_ctx);
    }
}

