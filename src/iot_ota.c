#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

#include <iot_crypto.h>
#include <iot_nvs.h>

#include "../include/iot_ota.h"
#include "internal.h"

static const char *TAG = "OTA";

static char *ota_server = NULL;
static uint8_t *ota_device_id = NULL;
static char *ota_firmware_id = NULL;

static void
ota_task (void *params);

static int
ota_check (char *out_firmware_hash, size_t out_size);

static esp_err_t
ota_download_and_update (const char *url);

void
ota_set_server (char *url) {
  ota_server = url;
}

static void
ota_init () {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(nvs_flash_init());

  nvs_create("ota");

  ota_device_id = nvs_read("ota", "id");

  if (ota_device_id == NULL) {
    ota_device_id = crypto_random_bytes(32);
    nvs_write("ota", "id", ota_device_id, sizeof(ota_device_id));
  }
}

static void
ota_set_firmware (const char *firmware_id) {
  if (firmware_id == NULL) {
    nvs_delete("ota", "firmware");
    return;
  }

  nvs_write_string("ota", "firmware", firmware_id);
}

static void
ota__ensure_firmware_exists () {
  ota_firmware_id = nvs_read_string("ota", "firmware");

  if (ota_firmware_id == NULL) {
    ESP_LOGE(TAG, "Firmware is not configured");
    ESP_ERROR_CHECK(ESP_FAIL);
    return;
  }
}

void
ota_updates (const char *firmware_id) {
  if (ota_server == NULL) {
    ota_set_server(OTA_DEFAULT_SERVER);
  }

  ota_init();

  if (firmware_id) {
    ota_set_firmware(firmware_id);
  }

  ota__ensure_firmware_exists();

  xTaskCreate(ota_task, "ota_task", 8192, NULL, 4, NULL);
}

static void
ota_task (void *params) {
  int retries = 0;

  while (1) {
    char out_firmware_hash[OTA_MAX_LENGTH_FIRMWARE_HASH + 1];
    int status_code = ota_check(out_firmware_hash, sizeof(out_firmware_hash));

    if (status_code == OTA_STATUS_NOT_FOUND) {
      ESP_LOGE(TAG, "Firmware not found");

      vTaskDelay(retry_delay(1000, 10, &retries) / portTICK_PERIOD_MS);
      continue;
    }

    // TODO: EventSource or long-polling
    if (status_code == OTA_STATUS_UP_TO_DATE) {
      ESP_LOGI(TAG, "Firmware is up to date");

      retries = 0;
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }

    if (status_code == OTA_STATUS_AVAILABLE) {
      ESP_LOGI(TAG, "Firmware available");

      char url[OTA_MAX_URL_LENGTH];
      snprintf(url, sizeof(url), "%s/v1/download/%s", ota_server, out_firmware_hash);

      esp_err_t err_download = ota_download_and_update(url);

      if (err_download != ESP_OK) {
        ESP_LOGE(TAG, "ota_download_and_update() Failed to update: %d", err_download);

        vTaskDelay(retry_delay(1000, 10, &retries) / portTICK_PERIOD_MS);
        continue;
      }

      retries = 0;

      nvs_write_string("ota", "hash", out_firmware_hash);

      // TODO: Allow manual restart to avoid interruption, and timeout to force it
      esp_restart();

      continue;
    }

    ESP_LOGE(TAG, "ota_check() Unknown status: %d", status_code);

    vTaskDelay(retry_delay(1000, 10, &retries) / portTICK_PERIOD_MS);
    continue;
  }
}

static int
ota_check (char *out_firmware_hash, size_t out_size) {
  char *current_hash = nvs_read_string("ota", "hash");

  char url[OTA_MAX_URL_LENGTH];
  snprintf(url, sizeof(url), "%s/v1/check/%s", ota_server, current_hash ? current_hash : "");

  free(current_hash);

  ESP_LOGI(TAG, "ota_check() %s", url);

  esp_http_client_config_t http_config = {
    .url = url,
    .transport_type = HTTP_TRANSPORT_OVER_SSL,
    .crt_bundle_attach = esp_crt_bundle_attach,
  };

  esp_http_client_handle_t client = esp_http_client_init(&http_config);

  if (client == NULL) {
    return -1;
  }

  char *device_id = bytes_to_hex(ota_device_id, 32);

  esp_http_client_set_header(client, "x-ota-device-id", device_id);
  esp_http_client_set_header(client, "x-ota-firmware-id", ota_firmware_id);

  free(device_id);

  esp_err_t err = esp_http_client_set_method(client, HTTP_METHOD_GET);

  if (err != ESP_OK) {
    esp_http_client_cleanup(client);
    return -2;
  }

  err = esp_http_client_open(client, 0);

  if (err != ESP_OK) {
    esp_http_client_cleanup(client);
    return -3;
  }

  int content_length = esp_http_client_fetch_headers(client);

  if (content_length < 0) {
    esp_http_client_cleanup(client);
    return -4;
  }

  int status_code = esp_http_client_get_status_code(client);

  if (status_code == OTA_STATUS_AVAILABLE) {
    int read_len = esp_http_client_read_response(client, out_firmware_hash, out_size - 1);

    if (read_len < 0) {
      out_firmware_hash[0] = '\0';
      esp_http_client_cleanup(client);
      return -5;
    }

    out_firmware_hash[read_len] = '\0';
  }

  esp_http_client_cleanup(client);

  return status_code;
}

static esp_err_t
_http_client_init_cb (esp_http_client_handle_t client) {
  char *device_id = bytes_to_hex(ota_device_id, 32);

  esp_http_client_set_header(client, "x-ota-device-id", device_id);
  esp_http_client_set_header(client, "x-ota-firmware-id", ota_firmware_id);

  free(device_id);

  return ESP_OK;
}

static esp_err_t
ota_download_and_update (const char *url) {
  esp_http_client_config_t http_config = {
    .url = url,
    .timeout_ms = 5 * 60 * 1000,
    .transport_type = HTTP_TRANSPORT_OVER_SSL,
    .crt_bundle_attach = esp_crt_bundle_attach,
  };

  esp_https_ota_config_t ota_config = {
    .http_config = &http_config,
    .http_client_init_cb = _http_client_init_cb,
  };

  ESP_LOGI(TAG, "ota_download_and_update() %s", url);

  esp_https_ota_handle_t handle = NULL;
  esp_err_t err = esp_https_ota_begin(&ota_config, &handle);

  if (err != ESP_OK) {
    esp_https_ota_abort(handle);
    return -1;
  }

  esp_err_t ota_perform_err;
  uint progress = 0;

  while (true) {
    ota_perform_err = esp_https_ota_perform(handle);

    if (ota_perform_err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
      if (progress++ % 32 == 0) {
        ESP_LOGI(TAG, "esp_https_ota_perform() Image bytes read: %d", esp_https_ota_get_image_len_read(handle));
      }

      continue;
    }

    break;
  }

  if (ota_perform_err != ESP_OK) {
    esp_https_ota_abort(handle);
    return -2;
  }

  if (!esp_https_ota_is_complete_data_received(handle)) {
    esp_https_ota_abort(handle);
    return -3;
  }

  esp_err_t ota_finish_err = esp_https_ota_finish(handle);

  if (ota_finish_err != ESP_OK) {
    return -4;
  }

  return ESP_OK;
}
