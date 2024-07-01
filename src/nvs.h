#ifndef OTA_NVS_H
#define OTA_NVS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void
nvs_create (const char *space);

void
nvs_purge (const char *space);

void
nvs_write (const char *space, const char *key, const uint8_t *value, size_t length);

void
nvs_write_string (const char *space, const char *key, const char *value);

void
nvs_write_bool (const char *space, const char *key, bool value);

uint8_t *
nvs_read (const char *space, const char *key);

char *
nvs_read_string (const char *space, const char *key);

bool
nvs_read_bool (const char *space, const char *key);

void
nvs_delete (const char *space, const char *key);

#ifdef __cplusplus
}
#endif

#endif // OTA_NVS_H
