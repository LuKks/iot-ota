#ifndef OTA_INTERNAL_H
#define OTA_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// #define OTA_DEFAULT_SERVER "https://ota.leet.ar"
#define OTA_DEFAULT_SERVER "http://192.168.1.24:7331"

#define OTA_STATUS_AVAILABLE  200
#define OTA_STATUS_UP_TO_DATE 204
#define OTA_STATUS_NOT_FOUND  404

#define OTA_MAX_LENGTH_SERVER_URL    128
#define OTA_MAX_LENGTH_FIRMWARE_ID   26
#define OTA_MAX_LENGTH_FIRMWARE_HASH 64
#define OTA_MAX_URL_LENGTH           (OTA_MAX_LENGTH_SERVER_URL + OTA_MAX_LENGTH_FIRMWARE_ID + OTA_MAX_LENGTH_FIRMWARE_HASH)

char *
bytes_to_hex (const uint8_t *buffer, size_t size);

int
pow_two (int n);

int
retry_delay (int delay, int max, int *retries);

#ifdef __cplusplus
}
#endif

#endif // OTA_INTERNAL_H
