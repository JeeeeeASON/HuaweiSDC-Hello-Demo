#include "log.h"
#include "sdc_client.h"
#include "event.h"
#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define LOG_ADDR_LEN 32
#define LOG_BUF_LEN 4096
#define LOG_FILE_LEN 128

struct log_ctx {
    struct sdc_client base;
    struct app_module module;
    char addr[LOG_ADDR_LEN];
    char buf[LOG_BUF_LEN];
    char file[LOG_FILE_LEN];
    int fd;
};

int g_app_log_level = LOG_DEBUG;

static int log_config_get(struct app_module* module, struct sdc_config_param* params, int cnt);
static int log_config_update(struct app_module* module, struct sdc_config_param* params, int cnt);

static struct log_ctx g_log_ctx = {
    .module = {
        .config_get = log_config_get,
        .config_update = log_config_update,
        .osd_connect = osd_simple_connect,
        .name = "log",
    },
};

static int log_on_connect(struct sdc_client* client, struct app_ctx* app_ctx);

static const struct sdc_client_operations g_log_ops = {
    .open = log_on_connect,
};

struct app_module* module_log_init(struct app_ctx* ctx)
{
    const char* log_server = getenv("hello_log_server");
    const char* log_file = getenv("hello_log_file");
    sdc_client_init(&g_log_ctx.base, "/mnt/srvfs/tproxy.paas.sdc", 0, &g_log_ops);

    if (log_file && *log_file) {
        snprintf(g_log_ctx.file, sizeof(g_log_ctx.file), "%s", log_file);
        g_log_ctx.fd = open(g_log_ctx.file, O_RDWR | O_CREAT);
        fprintf(stdout, "log_file: %s, fd: %d\n", log_file, g_log_ctx.fd);
    }else {
        g_log_ctx.fd = -1;
    }

    if(log_server && *log_server) {
        LOGD("connect log_server: %s", log_server);
        snprintf(g_log_ctx.addr, sizeof(g_log_ctx.addr), "%s", log_server);
        return sdc_client_connect(&g_log_ctx.base, ctx) ? 0 : &g_log_ctx.module;
    }
    LOGD("no log server...");
    return &g_log_ctx.module;
}

#define CONF_KEY_ADDR "log-server-addr"

static int log_config_get(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    struct log_ctx* log_ctx = container_of(module, struct log_ctx, module);

    if (cnt) {
        params[0].key = CONF_KEY_ADDR;
        params[0].value = log_ctx->addr;
        return 1 + module->next->config_get(module->next, params + 1, cnt - 1);
    }
    return 1 + module->next->config_get(module->next, 0, 0);
}

static void log_connect(struct log_ctx* log_ctx)
{
    struct sdc_client* client = &log_ctx->base;
    sdc_client_close(client, log_ctx->module.app);
    
    int nret = sdc_client_connect(client, log_ctx->module.app);
    if (nret) {
        fprintf(stderr, "log_connect to %s fail\n", log_ctx->addr);
    }
}

static int log_config_update(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    struct log_ctx* log_ctx = container_of(module, struct log_ctx, module);
    
    for (int i = 0; i < cnt; ++i) {
        if (strcmp(CONF_KEY_ADDR, params[i].key) == 0) {
            if (strcmp(log_ctx->addr, params[i].key)) {
                snprintf(log_ctx->addr, sizeof(log_ctx->addr), "%s", params[i].value);
                log_connect(log_ctx);
            }
            break;
        }
    }
    return module->next->config_update(module->next, params, cnt);
}

static int log_on_connect(struct sdc_client* client, struct app_ctx* app_ctx)
{
    struct log_ctx* log_ctx = container_of(client, struct log_ctx, base);
     struct sdc_tproxy_connection tproxy_addr = {
        .domain = AF_INET,
        .type = SOCK_STREAM,
     };

     struct sdc_common_head head = {
        .version = SDC_VERSION,
        .url = SDC_URL_TPROXY_CONNECTION,
        .method = SDC_METHOD_CREATE,
        .head_length = sizeof(head),
        .content_length = sizeof(tproxy_addr),
     };

     struct iovec iov[] = {
        { 
            .iov_base = &head, 
            .iov_len = sizeof(head)
        },
        { 
            .iov_base = &tproxy_addr, 
            .iov_len = sizeof(tproxy_addr) 
        },
     };

     snprintf(tproxy_addr.addr, sizeof(tproxy_addr.addr), "%s", log_ctx->addr);

     ssize_t ret = writev(sdc_client_fd(client), iov, sizeof(iov) / sizeof(iov[0]));
     if (ret == -1) {
        fprintf(stderr, "log on connect fail: %m\n");
        sdc_client_close(client, log_ctx->module.app);
        return ret;
     }
     return 0;
}

void app_log(int leve, const char* format, ...)
{
    int len;
    va_list args;
    va_start(args, format);
    len = vsnprintf(g_log_ctx.buf, sizeof(g_log_ctx.buf), format, args);
    va_end(args);

    ssize_t ret = write(sdc_client_fd(&g_log_ctx.base), g_log_ctx.buf, len);
    if (ret == -1 && errno == EBADF) {
        va_start(args, format);
        if (g_log_ctx.fd >= 0) {
            vdprintf(g_log_ctx.fd, format, args);
        }else {
            vfprintf(stdout, format, args);
        }
        va_end(args);
    }
}

