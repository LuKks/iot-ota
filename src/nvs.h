#ifndef OTA_NVS_H
#define OTA_NVS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void
ota_nvs_create (const char *space);

void
ota_nvs_erase_all (const char *space);

void
ota_nvs_erase_key (const char *space, const char *key);

void
ota_nvs_write_bytes (const char *space, const char *key, const uint8_t *value, size_t length);

void
ota_nvs_write_string (const char *space, const char *key, const char *value);

void
ota_nvs_write_bool (const char *space, const char *key, bool value);

uint8_t *
ota_nvs_read_bytes (const char *space, const char *key);

char *
ota_nvs_read_string (const char *space, const char *key);

bool
ota_nvs_read_bool (const char *space, const char *key);

#ifdef __cplusplus
}
#endif

#endif // OTA_NVS_H
