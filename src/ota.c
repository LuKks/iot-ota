#include "../include/ota.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "internal.h"
#include "nvs.h"
#include "random.h"
#include "root_ca.h"

#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>

static const char *TAG = "OTA";

static char *ota_server = NULL;
static char *ota_firmware_id = NULL;

static void
ota_task (void *params);

static void
ota__ensure_firmware_exists ();

static esp_err_t
http_client_init_cb (esp_http_client_handle_t client);

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

  xTaskCreate(ota_task, "ota_task", 4096, NULL, 4, NULL);
}

void
ota_set_server (const char *url) {
  ota_server = url;
}

void
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
ota_init () {
  nvs_create("ota");

  ota_device_id = nvs_read("ota", "id");

  if (ota_device_id == NULL) {
    ota_device_id = random_bytes(32);
    nvs_write("ota", "id", ota_device_id, sizeof(ota_device_id));
  }
}

static void
ota_task (void *params) {
  int retries = 0;

  while (1) {
    char out_firmware_hash[OTA_MAX_LENGTH_FIRMWARE_HASH + 1];
    int status_code = ota_check(&out_firmware_hash, sizeof(out_firmware_hash));

    vTaskDelay(retry_delay(1000, 10, &retries) / portTICK_PERIOD_MS);
    continue;

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
      snprintf(url, sizeof(url), "%s/v1/download/%s/%s", ota_server, ota_firmware_id, &out_firmware_hash);

      esp_err_t err_download = ota_update(url);

      if (err_download != ESP_OK) {
        ESP_LOGE(TAG, "Check request, failed to update: %d", err_download);
        vTaskDelay(retry_delay(1000, 10, &retries) / portTICK_PERIOD_MS);
        continue;
      }

      retries = 0;

      nvs_write_string("ota", "hash", &out_firmware_hash);

      // TODO: Allow manual restart to avoid interruption, and timeout to force it
      esp_restart();

      continue;
    }

    ESP_LOGE(TAG, "Check request, unknown status: %d", status_code);

    vTaskDelay(retry_delay(1000, 10, &retries) / portTICK_PERIOD_MS);

    continue;
  }
}

int
ota_check (char *out_firmware_hash, size_t out_size) {
  char *current_hash = nvs_read_string("ota", "hash");

  char url[OTA_MAX_URL_LENGTH];
  snprintf(url, sizeof(url), "%s/v1/check/%s", ota_server, current_hash ? current_hash : "");

  free(current_hash);

  esp_http_client_config_t config = {
    .url = url,
    .cert_pem = OTA_ROOT_CA,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  char *device_id = bytes_to_hex(ota_device_id, 32);

  esp_http_client_set_header(client, "x-ota-device-id", device_id);
  esp_http_client_set_header(client, "x-ota-firmware-id", ota_firmware_id);

  free(device_id);

  esp_err_t err = esp_http_client_perform(client);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Check request, failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return -1;
  }

  int status_code = esp_http_client_get_status_code(client);

  ESP_LOGE(TAG, "Check request, status code: %d", status_code);

  if (status_code == OTA_STATUS_AVAILABLE) {
    int read_len = esp_http_client_read_response(client, out_firmware_hash, out_size - 1);

    if (read_len < 0) {
      ESP_LOGE(TAG, "Check request, failed to read response");
      out_firmware_hash[0] = '\0';
      esp_http_client_cleanup(client);
      return -1;
    }

    out_firmware_hash[read_len] = '\0';
  } else {
    // Temporal debugging
    char buffer[128];
    int read_len = esp_http_client_read_response(client, buffer, sizeof(buffer) - 1);

    if (read_len < 0) {
      ESP_LOGE(TAG, "Check request, failed to read response (%d)", read_len);
      esp_http_client_cleanup(client);
      return -1;
    }

    buffer[read_len] = '\0';

    ESP_LOGE(TAG, "Check request, response (len %d): %s", read_len, buffer);
  }

  esp_http_client_cleanup(client);

  return status_code;
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
    .http_client_init_cb = http_client_init_cb,
  };

  // TODO: Use the advanced OTA APIs
  return esp_https_ota(&ota_config);
}

static esp_err_t
http_client_init_cb (esp_http_client_handle_t client) {
  char *device_id = bytes_to_hex(ota_device_id, 32);

  esp_http_client_set_header(client, "x-ota-device-id", device_id);
  esp_http_client_set_header(client, "x-ota-firmware-id", ota_firmware_id);

  free(device_id);

  return ESP_OK;
}
