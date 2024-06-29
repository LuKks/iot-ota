#include "../include/ota.h"

#include <ctype.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#define OTA_MAX_LENGTH_FIRMWARE_ID   26
#define OTA_MAX_LENGTH_FIRMWARE_HASH 64
#define OTA_MAX_LENGTH_SERVER_URL    128
#define OTA_MAX_URL_LENGTH           (OTA_MAX_LENGTH_FIRMWARE_ID + OTA_MAX_LENGTH_FIRMWARE_HASH + OTA_MAX_LENGTH_SERVER_URL)

char *ota_server = OTA_DEFAULT_SERVER;
char *ota_device_key = NULL;
bool _ota_verbose = false;

void
ota_init (const char *key) {
  ota_device_key = key;
}

void
ota_check (const char *firmware_id) {
  char *current_hash = ota_nvs_read_string("ota", "hash");
  char url[OTA_MAX_URL_LENGTH];

  snprintf(url, sizeof(url), "%s/v1/check/%s?hash=%s", ota_server, firmware_id, current_hash);

  esp_http_client_config_t config = {
    .url = url,
    .cert_pem = OTA_ROOT_CA,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_http_client_set_header(client, "x-ota-device", ota_device_key);

  esp_err_t err = esp_http_client_perform(client);

  if (err != ESP_OK) {
    ota_log("Failed HTTP request: %s", esp_err_to_name(err));
    goto done;
  }

  int status_code = esp_http_client_get_status_code(client);

  ota_log("OTA check, status code: %d", status_code);

  switch (status_code) {
  case OTA_STATUS_UP_TO_DATE:
    break;

  case OTA_STATUS_AVAILABLE:
    char new_hash[64 + 1] = {0};
    int read_len = esp_http_client_read_response(client, new_hash, sizeof(new_hash) - 1);

    if (read_len < 0) {
      ota_log("Failed to read response");
      break;
    }

    new_hash[read_len] = '\0';

    esp_err_t err_download = _ota_download(firmware_id, new_hash);

    if (err_download != ESP_OK) {
      ota_log("OTA failed to update: %d", err_download);
      break;
    }

    ota_nvs_write_string("ota", "hash", new_hash);

    // TODO: Allow manual restart to avoid interruption, and timeout to force it
    esp_restart();

    break;

  case OTA_STATUS_NOT_FOUND:
    ota_log("No new firmware available (404)");
    break;

  default:
    ota_log("Got unexpected status code %d", status_code);
    break;
  }

  goto done;

done:
  esp_http_client_cleanup(client);
}

void
ota_verbose (bool state) {
  _ota_verbose = state;
}

esp_err_t
_ota_download (char *firmware_id, char *new_hash) {
  char url[OTA_MAX_URL_LENGTH];

  snprintf(url, sizeof(url), "%s/v1/download/%s/%s", ota_server, firmware_id, new_hash);

  esp_http_client_config_t config = {
    .url = url,
    .cert_pem = OTA_ROOT_CA,
  };

  // TODO: Keep-alive, partial download, etc
  esp_https_ota_config_t ota_config = {
    .http_config = &config,
    .http_client_init_cb = _http_client_init_cb,
  };

  // TODO: Use the advanced OTA APIs
  return esp_https_ota(&ota_config);
}

esp_err_t
_http_client_init_cb (esp_http_client_handle_t client) {
  esp_http_client_set_header(client, "x-ota-device", ota_device_key);

  return ESP_OK;
}
