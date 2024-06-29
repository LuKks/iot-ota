#ifndef OTA_H
#define OTA_H

#include "./ota/log.h"
#include "./ota/nvs.h"
#include "./ota/root_ca.h"

#define OTA_ROOT_CA \
  OTA_ROOT_CA_LETS_ENCRYPT_R3 \
  OTA_ROOT_CA_GOOGLE_R1

#define OTA_DEFAULT_SERVER "https://ota.leet.ar"

#define OTA_STATUS_AVAILABLE  200
#define OTA_STATUS_UP_TO_DATE 204
#define OTA_STATUS_NOT_FOUND  404

// TODO: ESP_ERROR_CHECK but esp_restart() instead of abort()

void
ota_init (const char *api_key);

void
ota_check (const char *firmware_id);

void
ota_verbose (bool state);

#endif // OTA_H
