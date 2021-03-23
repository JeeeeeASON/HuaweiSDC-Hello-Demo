
#ifndef MODULE_OSD_H
#define MODULE_OSD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif 

struct app_ctx;

struct app_module* module_osd_init(struct app_ctx* app_ctx);

void osd_update(const char* key, const char* value);
void osd_update_uint(const char* key, uint32_t val);
void osd_name_register(const char* key);
void osd_desc_register(const char* key, const char* desc, uint32_t lang);

#ifdef __cplusplus
}
#endif

#endif

