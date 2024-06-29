#ifndef OTA_NVS_H
#define OTA_NVS_H

void
ota_nvs_init ();

void
ota_nvs_write_string (const char *namespace, const char *key, const char *value);

char *
ota_nvs_read_string (const char *namespace, const char *key);

#endif // OTA_NVS_H
