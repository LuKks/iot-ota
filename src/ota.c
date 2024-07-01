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

char *ota_server = OTA_DEFAULT_SERVER;
char *ota_firmware_id = NULL;

static void
ota_task (void *params);

static bool
ota_signup ();

static esp_err_t
http_client_init_cb (esp_http_client_handle_t client);

void
ota_set_server (const char *url) {
  ota_server = url;
}

void
ota_init (const char *firmware_id) {
  nvs_create("ota");
  // nvs_purge("ota");

  ota_firmware_id = firmware_id;

  ota_device_id = nvs_read("ota", "id");
  ota_device_key = nvs_read("ota", "key");

  if (ota_device_id == NULL) {
    ota_device_id = random_bytes(32);

    nvs_write("ota", "id", ota_device_id, sizeof(ota_device_id));
  }

  if (ota_device_key == NULL) {
    nvs_write_bool("ota", "signup", false);

    ota_device_key = random_bytes(32);
    nvs_write("ota", "key", ota_device_key, sizeof(ota_device_key));
  }

  xTaskCreate(ota_task, "ota_task", 4096, NULL, 4, NULL);
}

static void
ota_task (void *params) {
  bool registered = nvs_read_bool("ota", "signup");

  while (1) {
    if (!registered) {
      int retries = 0;

      while (1) {
        bool success = ota_signup();

        if (!success) {
          ESP_LOGE(TAG, "Device could not register");
          vTaskDelay(retry_delay(1000, 10, &retries) / portTICK_PERIOD_MS);
          // continue;

          vTaskDelete(NULL);
          return;
        }

        nvs_write_bool("ota", "signup", true);
        registered = true;

        break;
      }
    }

    ESP_LOGE(TAG, "Device already sent the registration");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    continue;

    char out_firmware_hash[OTA_MAX_LENGTH_FIRMWARE_HASH + 1];
    int status_code = ota_check(&out_firmware_hash, sizeof(out_firmware_hash));

    if (status_code == OTA_STATUS_NOT_FOUND) {
      ESP_LOGE(TAG, "Firmware not found");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    if (status_code == OTA_STATUS_UP_TO_DATE) {
      ESP_LOGI(TAG, "Firmware is up to date");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    if (status_code == OTA_STATUS_AVAILABLE) {
      ESP_LOGI(TAG, "Firmware available");

      char url[OTA_MAX_URL_LENGTH];
      snprintf(url, sizeof(url), "%s/v1/download/%s/%s", ota_server, ota_firmware_id, out_firmware_hash);

      esp_err_t err_download = ota_update(url);

      if (err_download != ESP_OK) {
        ESP_LOGE(TAG, "Check request, failed to update: %d", err_download);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      }

      nvs_write_string("ota", "hash", out_firmware_hash);

      // TODO: Allow manual restart to avoid interruption, and timeout to force it
      esp_restart();
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    continue;
  }
}

static bool
ota_signup () {
  char *device_id = bytes_to_hex(ota_device_id, 32);
  char *device_key = bytes_to_hex(ota_device_key, 32);

  char data[192];
  snprintf(data, sizeof(data), "{\"id\":\"%s\",\"key\":\"%s\"}", device_id, device_key);

  free(device_id);
  free(device_key);

  char url[OTA_MAX_URL_LENGTH];
  snprintf(url, sizeof(url), "%s/v1/device/signup", ota_server);

  ESP_LOGE(TAG, "ota_signup() %s %s", url, data);

  esp_http_client_config_t config = {
    .url = url,
    .cert_pem = OTA_ROOT_CA,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, data, strlen(data));

  esp_err_t err = esp_http_client_perform(client);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Signup request, failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return false;
  }

  int status_code = esp_http_client_get_status_code(client);

  ESP_LOGE(TAG, "Signup request, status code: %d", status_code);

  if (status_code != 201) {
    ESP_LOGE(TAG, "Signup request, unexpected status code %d", status_code);
    esp_http_client_cleanup(client);
    return false;
  }

  esp_http_client_cleanup(client);

  return true;
}

int
ota_check (char *out_firmware_hash, size_t out_size) {
  char *current_hash = nvs_read_string("ota", "hash");

  char url[OTA_MAX_URL_LENGTH];
  snprintf(url, sizeof(url), "%s/v1/check/%s?hash=%s", ota_server, ota_firmware_id, current_hash);

  free(current_hash);

  ESP_LOGE(TAG, "ota_check() %s", url);

  esp_http_client_config_t config = {
    .url = url,
    .cert_pem = OTA_ROOT_CA,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  char *device_id = bytes_to_hex(ota_device_id, 32);
  char *device_key = bytes_to_hex(ota_device_key, 32);

  esp_http_client_set_header(client, "x-ota-device-id", device_id);
  esp_http_client_set_header(client, "x-ota-device-key", device_key);

  free(device_id);
  free(device_key);

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
  char *device_key = bytes_to_hex(ota_device_key, 32);

  esp_http_client_set_header(client, "x-ota-device-id", device_id);
  esp_http_client_set_header(client, "x-ota-device-key", device_key);

  free(device_id);
  free(device_key);

  return ESP_OK;
}
