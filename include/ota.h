#ifndef OTA_H
#define OTA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../src/root_ca.h"

#include <stdbool.h>

#include <esp_err.h>

#define OTA_ROOT_CA \
  OTA_ROOT_CA_LETS_ENCRYPT_R3 \
  OTA_ROOT_CA_GOOGLE_R1

void
ota_init (const char *api_key);

void
ota_check (const char *firmware_id);

esp_err_t
ota_update (const char *url);

void
ota_verbose (bool state);

#ifdef __cplusplus
}
#endif

#endif // OTA_H
