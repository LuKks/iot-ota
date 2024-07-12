#ifndef IOT_OTA_H
#define IOT_OTA_H

#ifdef __cplusplus
extern "C" {
#endif

void
ota_set_server (char *url);

void
ota_updates (const char *firmware_id);

#ifdef __cplusplus
}
#endif

#endif // IOT_OTA_H
