
#include "gateway.h"
#include "log.h"
#include "osd.h"
#include "sdc_client.h"
#include "sdc.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

struct gateway_ctx {
    struct sdc_client base;
    struct app_module module;
};

static int gateway_config_get(struct app_module* module, struct sdc_config_param* params, int cnt);
static int gateway_config_update(struct app_module* module, struct sdc_config_param* params, int cnt);
static void gateway_osd_connect(struct app_module* module);

static struct gateway_ctx g_gateway_ctx = {
    .module = {
        .config_get = gateway_config_get,
        .config_update = gateway_config_update,
        .osd_connect = gateway_osd_connect,
        .name = "gateway",
    },
};

static const struct sdc_client_operations g_gateway_ops;

struct app_module* module_gateway_init(struct app_ctx* app_ctx)
{
    const char* name = getenv("hello_app_name");
    sdc_client_init(&g_gateway_ctx.base, "/mnt/srvfs/gateway.paas.sdc", 0, &g_gateway_ops);

    if (name && *name) {
        snprintf(app_ctx->name, sizeof(app_ctx->name), "%s", name);
    }
    return sdc_client_connect(&g_gateway_ctx.base, app_ctx) ? 0 : &g_gateway_ctx.module;
}

#define CONF_KEY_APP_NAME "hello_name"
#define OSD_KEY_APP_NAME "${hello_name}"
#define OSD_KEY_APP_DESC "应用名称"

static void gateway_osd_connect(struct app_module* module)
{
    osd_name_register(OSD_KEY_APP_NAME);
    osd_desc_register(OSD_KEY_APP_NAME, OSD_KEY_APP_DESC, SDC_OSD_LANG_ZH);
    osd_update(OSD_KEY_APP_NAME, module->app->name);
    module->next->osd_connect(module->next);
}

static int gateway_config_get(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    if (cnt) {
        params[0].key = CONF_KEY_APP_NAME;
        params[0].value = module->app->name;
        return module->next->config_get(module->next, params + 1, cnt - 1) + 1;
    }
    return 1 + module->next->config_get(module->next, 0, 0);
}

static int gateway_config_update(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    int i = 0;

    for (; i < cnt; ++i) {
        if (strcmp(CONF_KEY_APP_NAME, params[i].key) == 0) {
            if (strcmp(params[i].key, module->app->name) != 0) {
                snprintf(module->app->name, sizeof(module->app->name), "%s", params[i].value);
                osd_update(OSD_KEY_APP_NAME, module->app->name);
            }
            break;
        }
    }
    return module->next->config_update(module->next, params, cnt);
}

static int app_gateway_connect(struct sdc_client* client, struct app_ctx* app_ctx)
{
    const char* usr_conf = "./portal.conf";
    struct paas_gateway_conf conf;
    struct sdc_common_head head = {
        .version = SDC_VERSION,
        .url = SDC_URL_PAAS_GATEWAY_CONF,
        .method = SDC_METHOD_CREATE,
        .head_length = sizeof(head),
        .content_length = sizeof(conf),
    };

    struct iovec iov[2] = {
        { .iov_base = &head, .iov_len = sizeof(head) },
        { .iov_base = &conf, .iov_len = sizeof(conf) },
    };

    snprintf(conf.path, sizeof(conf.path), "%s", usr_conf);
    LOGI("register gateway.conf: %s ...", conf.path);

    ssize_t ret = writev(sdc_client_fd(client), iov, sizeof(iov) / sizeof(iov[0]));

    if (ret == -1) {
        LOGE("register gateway.conf fail: %s: %m", conf.path);
        return ret;
    }
    return 0;
}

static void app_gateway_response(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx)
{
    if (head->code == SDC_CODE_200) {
        LOGI("register gateway.conf return success");
    }else {
        LOGE("register gateway.conf return fail: %d", head->code);
    }
}

static const struct sdc_client_operations g_gateway_ops = {
    .open = app_gateway_connect,
    .handle_response = app_gateway_response,
};

