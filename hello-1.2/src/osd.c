
#include "osd.h"
#include "sdc_client.h"
#include "log.h"
#include "sdc.h"
#include "module.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>

struct module_osd_ctx {
    struct sdc_client base;
    struct app_module module;
};

static struct module_osd_ctx g_osd_ctx = {
    .module = APP_NULL_MODULE("osd"),
};

static int osd_connect(struct sdc_client* client, struct app_ctx* app_ctx);
static void osd_close(struct sdc_client* client, struct app_ctx* app_ctx);
static void osd_handle_response(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx);
static void osd_handle_error(struct sdc_client* client, int err, struct app_ctx* app_ctx);

static const struct sdc_client_operations g_osd_ops = {
    .open = osd_connect,
    .close = osd_close,
    .handle_response = osd_handle_response,
    .handle_error = osd_handle_error,
};

struct app_module* module_osd_init(struct app_ctx* app_ctx)
{
    sdc_client_init(&g_osd_ctx.base, "/mnt/srvfs/osd.iaas.sdc", 0, &g_osd_ops);
    return sdc_client_connect(&g_osd_ctx.base, app_ctx) ? 0 : &g_osd_ctx.module;
}

static int osd_connect(struct sdc_client* client, struct app_ctx* app_ctx)
{
    app_ctx->module_list->osd_connect(app_ctx->module_list);
    return 0;
}

static void osd_close(struct sdc_client* client, struct app_ctx* app_ctx)
{
    LOGE("osd sdc connection closed");
}

static void osd_handle_error(struct sdc_client* client, int err, struct app_ctx* app_ctx)
{
    LOGE("osd module fail,err: %d, %m", err);
    sdc_client_close(client, app_ctx);
}


static void osd_handle_response(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx)
{
    if (head->code != SDC_CODE_200) {
        LOGE("osd: errcode of response: %d", head->code);
        sdc_client_close(client, app_ctx);
        return;
    }
    switch (head->url) {
        case SDC_URL_OSD_ENV_NAME:
            LOGD("register osd name success!\n");
            break;
        case SDC_URL_OSD_ENV_DESC:
            LOGD("register osd desc success!\n");
            break;
        case SDC_URL_OSD_ENV_VALUE:
            LOGD("update osd value success!\n");
            break;
        default:
            LOGD("unknown osd response!\n");
            break;
    };
}

void osd_update(const char* key, const char* value)
{
    struct sdc_client* client = &g_osd_ctx.base;
    struct sdc_osd_env_value osd_value;
    struct sdc_common_head head = {
        .version = SDC_VERSION,
        .url = SDC_URL_OSD_ENV_VALUE,
        .method = SDC_METHOD_UPDATE,
        .head_length = sizeof(head),
        .content_length = sizeof(osd_value),
    };
    struct iovec iov[2];

    iov[0].iov_base = &head;
    iov[0].iov_len = sizeof(head);
    iov[1].iov_base = &osd_value;
    iov[1].iov_len = sizeof(osd_value);

    snprintf(osd_value.name, sizeof(osd_value.name), "%s", key);
    snprintf(osd_value.value, sizeof(osd_value.value), "%s", value);
    LOGI("osd_update: %s=%s ...", osd_value.name, osd_value.value);

    ssize_t ret = writev(sdc_client_fd(client), iov, sizeof(iov) / sizeof(iov[0]));
    if (ret == -1) {
        LOGE("osd_update fail: %s=%s, %m", osd_value.name, osd_value.value);
        sdc_client_close(client, g_osd_ctx.module.app);
    }
        
}

void osd_update_uint(const char* key, uint32_t val)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%u", val);
    osd_update(key, buf);
}

void osd_name_register(const char* key)
{
    struct sdc_client* client = &g_osd_ctx.base;

    static struct sdc_common_head head = {
        .version = SDC_VERSION,
        .url = SDC_URL_OSD_ENV_NAME,
        .method = SDC_METHOD_CREATE,
        .head_length = sizeof(head),
        .content_length = sizeof(struct sdc_osd_env_name),
    };
    struct sdc_osd_env_name osd_name;
    struct iovec iov[2];

    snprintf(osd_name.name, sizeof(osd_name.name), "%s", key);

    iov[0].iov_base = &head;
    iov[0].iov_len = sizeof(head);
    iov[1].iov_base = &osd_name;
    iov[1].iov_len = sizeof(osd_name);

    LOGI("osd_name_register %s ...", key);
    ssize_t ret = writev(sdc_client_fd(client), iov, sizeof(iov) / sizeof(iov[0]));
    if (ret == -1) {
        LOGE("osd_name_register %s fail: %m", key);
        sdc_client_close(client, g_osd_ctx.module.app);
    }
}

void osd_desc_register(const char* key, const char* desc, uint32_t lang)
{
    struct sdc_client* client = &g_osd_ctx.base;
    static struct sdc_common_head head = {
        .version = SDC_VERSION,
        .url = SDC_URL_OSD_ENV_DESC,
        .method = SDC_METHOD_CREATE,
        .head_length = sizeof(head),
        .content_length = sizeof(struct sdc_osd_env_desc),
    };
    struct sdc_osd_env_desc osd_desc;
    struct iovec iov[2];

    snprintf(osd_desc.name, sizeof(osd_desc.name), "%s", key);
    snprintf(osd_desc.desc, sizeof(osd_desc.desc), "%s", desc);
    osd_desc.lang = lang;

    iov[0].iov_base = &head;
    iov[0].iov_len = sizeof(head);
    iov[1].iov_base = &osd_desc;
    iov[1].iov_len = sizeof(osd_desc);

    LOGI("osd_desc_register %s,%s,%u ...", key, desc, lang);
    ssize_t ret = writev(sdc_client_fd(client), iov, sizeof(iov) / sizeof(iov[0]));
    if (ret == -1) {
        LOGE("osd_desc_register %s,%s,%u fail:  %m", key, desc, lang);
        sdc_client_close(client, g_osd_ctx.module.app);
    }
}

