#ifndef OTA_NVS_H
#define OTA_NVS_H

#ifdef __cplusplus
extern "C" {
#endif

void
ota_nvs_init ();

void
ota_nvs_write_string (const char *namespace, const char *key, const char *value);

char *
ota_nvs_read_string (const char *namespace, const char *key);

#ifdef __cplusplus
}
#endif

#endif // OTA_NVS_H
