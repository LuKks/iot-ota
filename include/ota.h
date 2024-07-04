#ifndef OTA_H
#define OTA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <stdbool.h>

uint8_t *ota_device_id;

void
ota_set_server (const char *url);

void
ota_updates (const char *firmware_id);

int
ota_check (char *out_firmware_hash, size_t out_size);

esp_err_t
ota_update (const char *url);

#ifdef __cplusplus
}
#endif

#endif // OTA_H
