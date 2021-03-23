#ifndef APP_LOG_H
#define APP_LOG_H

#include <syslog.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "module.h"

#ifdef __cplusplus
extern "C" {
#endif

struct app_module* module_log_init(struct app_ctx* ctx);

void app_log(int leve, const char* format, ...);


#define app_gettid() syscall(__NR_gettid)

extern int g_app_log_level;

#define APP_LOG(level, format, ...) \
    do { \
        if ((level) <= g_app_log_level) { \
            app_log(level, "%d|%d:%d:%s:%d|" format "\n", level, (int)getpid(), (int)app_gettid(), __FILE__,__LINE__, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOGD(format, ...) APP_LOG(LOG_DEBUG, format, ##__VA_ARGS__)
#define LOGI(format, ...) APP_LOG(LOG_INFO, format, ##__VA_ARGS__)
#define LOGW(format, ...) APP_LOG(LOG_WARNING, format, ##__VA_ARGS__)
#define LOGE(format, ...) APP_LOG(LOG_ERR, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif

