
#include "meta.h"
#include "log.h"
#include "sdc_client.h"
#include "sdc.h"
#include "osd.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>

struct tlv {
    uint32_t type;
    uint32_t len;
    char val[0];
};

struct module_metadata_ctx {
    struct sdc_client base;
    struct app_module module;
    /** 车牌的相关信息 */
    char car_number[32];
    char last_car[64];
    char str_count[32];
    char str_all_count[32];
    uint32_t count;
    uint32_t all_count;
};

static int meta_config_get(struct app_module* module, struct sdc_config_param* params, int cnt);
static int meta_config_update(struct app_module* module, struct sdc_config_param* params, int cnt);
static void meta_osd_connect(struct app_module* module);

static struct module_metadata_ctx g_metadata_ctx = {
    .module = {
        .config_get = meta_config_get,
        .config_update = meta_config_update,
        .osd_connect = meta_osd_connect,
        .name = "metadata",
    },
    .str_count = "0",
    .str_all_count = "0",
};

static const struct sdc_client_operations g_metadata_ops;

struct app_module* module_metadata_init(struct app_ctx* app_ctx)
{
    sdc_client_init(&g_metadata_ctx.base, "/mnt/srvfs/event.paas.sdc", 0, &g_metadata_ops);
    return sdc_client_connect(&g_metadata_ctx.base, app_ctx) ? 0 : &g_metadata_ctx.module;
}


#define CONF_KEY_CARNUMBER "hello-car-number"
#define CONF_KEY_LASTCAR  "hello-last-car"
#define CONF_KEY_COUNTER  "hello-count"
#define CONF_KEY_ALL_COUNTER "hello-all-count"

#define OSD_DESC_CARNUMBER "指定车牌号"
#define OSD_DESC_LASTCAR  "最近车牌号"
#define OSD_DESC_COUNTER  "指定车辆出现次数"
#define OSD_DESC_ALL_COUNTER "所有车辆出现次数"

#define OSD_KEY_CARNUMBER "${hello-car-number}"
#define OSD_KEY_LASTCAR   "${hello-last-car}"
#define OSD_KEY_COUNTER   "${hello-count}"
#define OSD_KEY_ALL_COUNTER "${hello-all-count}"

static int meta_config_get(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    struct module_metadata_ctx* meta_ctx = container_of(module, struct module_metadata_ctx, module);

    switch (cnt) {
        default:
        case 4:
            params[0].key = CONF_KEY_CARNUMBER;
            params[0].value = meta_ctx->car_number;
            --cnt; ++params;
            /** 继续后续处理，不用break */
        case 3:
            params[0].key = CONF_KEY_COUNTER;
            params[0].value = meta_ctx->str_count;
            --cnt; ++params;
            /** 继续后续处理，不用break */
        case 2:
            params[0].key = CONF_KEY_LASTCAR;
            params[0].value = meta_ctx->last_car;
            --cnt; ++params;
            /** 继续后续处理，不用break */
        case 1:
            params[0].key = CONF_KEY_ALL_COUNTER;
            params[0].value = meta_ctx->str_all_count;
            --cnt; ++params;
            /** 继续后续处理，不用break */
        case 0:
            break;
    }
    return 4 + module->next->config_get(module->next, params, cnt);
}

static int meta_config_update(struct app_module* module, struct sdc_config_param* params, int cnt)
{
    struct module_metadata_ctx* meta_ctx = container_of(module, struct module_metadata_ctx, module);

    for (int i = 0; i < cnt; ++i) {
        if (strcmp(params[i].key, CONF_KEY_CARNUMBER) == 0) {
            if (strcmp(params[i].value, meta_ctx->car_number) != 0) {
                snprintf(meta_ctx->car_number, sizeof(meta_ctx->car_number), "%s", params[i].value);
                osd_update(OSD_KEY_CARNUMBER, meta_ctx->car_number);
            }
        }
    }
    return module->next->config_update(module->next, params, cnt);
}

static void meta_osd_connect(struct app_module* module)
{
    struct module_metadata_ctx* meta_ctx = container_of(module, struct module_metadata_ctx, module);

    osd_name_register(OSD_KEY_CARNUMBER);
    osd_name_register(OSD_KEY_LASTCAR);
    osd_name_register(OSD_KEY_COUNTER);
    osd_name_register(OSD_KEY_ALL_COUNTER);

    osd_desc_register(OSD_KEY_CARNUMBER, OSD_DESC_CARNUMBER, SDC_OSD_LANG_ZH);
    osd_desc_register(OSD_KEY_LASTCAR, OSD_DESC_LASTCAR, SDC_OSD_LANG_ZH);
    osd_desc_register(OSD_KEY_COUNTER, OSD_DESC_COUNTER, SDC_OSD_LANG_ZH);
    osd_desc_register(OSD_KEY_ALL_COUNTER, OSD_DESC_ALL_COUNTER, SDC_OSD_LANG_ZH);

    osd_update(OSD_KEY_COUNTER, meta_ctx->str_count);
    osd_update(OSD_KEY_ALL_COUNTER, meta_ctx->str_all_count);
}

static int metadata_connect(struct sdc_client* client, struct app_ctx* app_ctx);
static void metadata_close(struct sdc_client* client, struct app_ctx* app_ctx);
static void metadata_handle_response(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx);
static void metadata_handle_error(struct sdc_client* client, int err, struct app_ctx* app_ctx);

static const struct sdc_client_operations g_metadata_ops = {
    .open = metadata_connect,
    .close = metadata_close,
    .handle_response = metadata_handle_response,
    .handle_error = metadata_handle_error,
};


static int metadata_connect(struct sdc_client* client, struct app_ctx* app_ctx)
{
    struct paas_event_filter filters[] = {
        { 
            .subscriber = "hello app",
            .name = "itgt.saas.sdc",
        },
    };

    struct sdc_common_head head = {
        .version = SDC_VERSION,
        .url = SDC_URL_PAAS_EVENTD_EVENT,
        .method = SDC_METHOD_GET,
        .head_length = sizeof(head),
        .content_length = sizeof(filters),
    };

    struct iovec iov[2];
    
    iov[0].iov_base = &head;
    iov[0].iov_len = sizeof(head);
    iov[1].iov_base = (void*)&filters;
    iov[1].iov_len = sizeof(filters);

    LOGI("metadata subscribe ...");
    ssize_t ret = writev(sdc_client_fd(client), iov, sizeof(iov) / sizeof(iov[0]));

    if (ret == -1) {
        /** 出错了，可能是服务端故障, 关闭后会自动重连 */
        LOGE("metadata subscribe fail: %m");
        sdc_client_close(client, app_ctx);
        return errno;
    }
    return 0;
}

static void metadata_close(struct sdc_client* client, struct app_ctx* app_ctx)
{
    LOGI("metadata connection closed ...");
}

static void metadata_handle_error(struct sdc_client* client, int err, struct app_ctx* app_ctx)
{
    LOGE("metadata module fail,err: %d, %m\n", err);
    sdc_client_close(client, app_ctx);
}


static void* shm_mmap(struct paas_shm_cached_event* shm);
static void shm_munmap(struct paas_shm_cached_event* shm, void* virt_addr);
static void metadata_process(struct sdc_client* client, struct paas_event* paas_event, uint32_t len, struct app_ctx* app_ctx);

static void metadata_handle_response(struct sdc_client* client, struct sdc_common_head* head, struct app_ctx* app_ctx)
{
    if (head->code != SDC_CODE_200) {
        LOGE("metadata errcode of response: %d", head->code);
        sdc_client_close(client, app_ctx);
        return;
    }

    struct paas_event* paas_event;
    struct sdc_extend_head* extend;

    sdc_for_each_extend_head(head, extend) {
        if (extend->type == SDC_HEAD_SHM_CACHED_EVENT) {
            struct paas_shm_cached_event* shm;
            if (head->content_length != sizeof(*shm)) {
                return;
            }
            shm = (struct paas_shm_cached_event*)((char*)head + head->head_length);
            paas_event = shm_mmap(shm);
            if (paas_event) {
                metadata_process(client, paas_event, shm->size, app_ctx);
                shm_munmap(shm, paas_event);
            }
            return;
        }
    }

    paas_event = (struct paas_event*)((char*)head + head->head_length);
    metadata_process(client, paas_event, head->content_length, app_ctx);
}

static void* shm_mmap(struct paas_shm_cached_event* shm)
{
    static int fd = -1;
    if (fd == -1) {
        fd = open("/dev/cache", O_RDWR | O_CLOEXEC);
    }
    if (fd == -1) return 0;

    struct sdc_shm_cache cache = {
        .addr_phy = shm->addr_phy,
        .size = shm->size,
        .cookie = shm->cookie,
    };

    int nret = ioctl(fd, SDC_CACHE_MMAP, &cache);
    if (nret) {
        return 0;
    }

    return cache.addr_virt;
}

static void shm_munmap(struct paas_shm_cached_event* shm, void* virt_addr)
{
    sdc_shm_cache_free(virt_addr, shm->size);
}

#define TLV_NEXT(tlv) ((void*)((tlv)->val + (tlv)->len))

#define tlv_for_each(tlv, item, end) \
        for ((item) = (typeof(item))(tlv); (void*)(item) < (void*)(end) && TLV_NEXT(item) <= (void*)(end); (item) = TLV_NEXT(item))

#define TLV_CAR_NUMBER 0x0A000008

static void metadata_process(struct sdc_client* client, struct paas_event* paas_event, uint32_t len, struct app_ctx* app_ctx)
{
    struct module_metadata_ctx* module = container_of(client, struct module_metadata_ctx, base);

    if (len <= sizeof(*paas_event)) {
        return;
    }

    if (len != sizeof(*paas_event) + paas_event->length) {
        return;
    }

    /** 三层TLV，车牌号在最里层 */
    struct tlv* tlv_1 = (struct tlv*)paas_event->data;
    struct tlv* tlv_1_end = (struct tlv*)&paas_event->data[paas_event->length];
    struct tlv* tlv_2;
    struct tlv* tlv_3;
    int has_car = 0;

    tlv_for_each (tlv_1->val, tlv_2, tlv_1_end) {
        tlv_for_each (tlv_2->val, tlv_3, TLV_NEXT(tlv_2)) {
            LOGD("tlv3_type: %X, len: %d", tlv_3->type, tlv_3->len);
            if (tlv_3->type == TLV_CAR_NUMBER) {
                has_car = 1;
                ++module->all_count;
                snprintf(module->last_car, sizeof(module->last_car), "%.*s", tlv_3->len, tlv_3->val);
                LOGI("car nubmer: %s, last: %s", module->car_number, module->last_car);
                if (strcmp(module->last_car, module->car_number) == 0) {
                    ++module->count;
                    snprintf(module->str_count, sizeof(module->str_count), "%u", module->count);
                    osd_update(OSD_KEY_COUNTER, module->str_count);
                }
            }
        }
    }

    if (has_car) {
        osd_update(OSD_KEY_LASTCAR, module->last_car);
        snprintf(module->str_all_count, sizeof(module->str_all_count), "%u", module->all_count);
        osd_update(OSD_KEY_ALL_COUNTER, module->str_all_count);
    }
}

