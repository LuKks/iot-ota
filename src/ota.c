#include "../include/ota.h"

#include "internal.h"
#include "log.h"
#include "nvs.h"

#include <esp_http_client.h>
#include <esp_https_ota.h>

char *ota_server = OTA_DEFAULT_SERVER;
char *ota_device_key = NULL;

esp_err_t
_http_client_init_cb (esp_http_client_handle_t client);

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

  if (ota_device_key != NULL) {
    esp_http_client_set_header(client, "x-ota-device", ota_device_key);
  }

  esp_err_t err = esp_http_client_perform(client);

  if (err != ESP_OK) {
    ota_log("Failed HTTP request: %s", esp_err_to_name(err));
    goto done;
  }

  int status_code = esp_http_client_get_status_code(client);

  ota_log("OTA check, status code: %d", status_code);

  if (status_code == OTA_STATUS_NOT_FOUND) {
    ota_log("No new firmware available (404)");
    goto done;
  }

  if (status_code == OTA_STATUS_UP_TO_DATE) {
    goto done;
  }

  if (status_code == OTA_STATUS_AVAILABLE) {
    char new_hash[64 + 1] = {0};
    int read_len = esp_http_client_read_response(client, new_hash, sizeof(new_hash) - 1);

    if (read_len < 0) {
      ota_log("Failed to read response");
      goto done;
    }

    new_hash[read_len] = '\0';

    snprintf(url, sizeof(url), "%s/v1/download/%s/%s", ota_server, firmware_id, new_hash);

    esp_err_t err_download = ota_update(url);

    if (err_download != ESP_OK) {
      ota_log("OTA failed to update: %d", err_download);
      goto done;
    }

    ota_nvs_write_string("ota", "hash", new_hash);

    // TODO: Allow manual restart to avoid interruption, and timeout to force it
    esp_restart();

    goto done;
  }

  ota_log("Got unexpected status code %d", status_code);

  goto done;

done:
  esp_http_client_cleanup(client);
}

esp_err_t
ota_update (const char *url) {
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
  if (ota_device_key != NULL) {
    esp_http_client_set_header(client, "x-ota-device", ota_device_key);
  }

  return ESP_OK;
}
