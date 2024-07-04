#ifndef OTA_H
#define OTA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <stdbool.h>

void
ota_set_server (char *url);

void
ota_updates (const char *firmware_id);

#ifdef __cplusplus
}
#endif

#endif // OTA_H
