#ifndef OTA_H
#define OTA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../src/root_ca.h"

#include <esp_err.h>
#include <stdbool.h>

#define OTA_ROOT_CA \
  OTA_ROOT_CA_LETS_ENCRYPT_R3 \
  OTA_ROOT_CA_GOOGLE_R1

bool ota_verbose;

void
ota_init (const char *api_key);

void
ota_check (const char *firmware_id);

esp_err_t
ota_update (const char *url);

#ifdef __cplusplus
}
#endif

#endif // OTA_H
