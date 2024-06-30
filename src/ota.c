#include "../include/ota.h"

#include "internal.h"
#include "nvs.h"
#include "random.h"
#include "root_ca.h"

#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OTA";

char *ota_server = OTA_DEFAULT_SERVER;
char *ota_firmware_id = NULL;

static esp_err_t
http_client_init_cb (esp_http_client_handle_t client);

static void
ota_signup ();

static void
bytes_to_hex_fill (const uint8_t *buffer, size_t size, char *hex);

void
ota_set_server (const char *url) {
  ota_server = url;
}

void
ota_init (const char *firmware_id) {
  ota_nvs_create("ota");

  ota_firmware_id = firmware_id;

  ota_device_id = ota_nvs_read_bytes("ota", "id");
  ota_device_key = ota_nvs_read_bytes("ota", "key");

  if (ota_device_id == NULL) {
    ota_device_id = random_bytes(32);
    ota_nvs_write_bytes("ota", "id", ota_device_id, sizeof(ota_device_id));
  }

  if (ota_device_key == NULL) {
    ota_nvs_write_bool("ota", "signup", false);

    ota_device_key = random_bytes(32);
    ota_nvs_write_bytes("ota", "key", ota_device_key, sizeof(ota_device_key));
  }

  xTaskCreate(ota_task, "ota_task", 4096, NULL, 4, NULL);
}

static void
ota_task (void *pvParameters) {
  bool registered = ota_nvs_read_bool("ota", "signup");

  if (registered == false) {
    bool success = ota_signup();

    if (success) {
      ota_nvs_write_bool("ota", "signup", true);
      registered = true;
    }
  }

  
}

static bool
ota_signup () {
  char *device_id = bytes_to_hex(ota_device_id, sizeof(ota_device_id));
  char *device_key = bytes_to_hex(ota_device_key, sizeof(ota_device_key));

  char data[256];
  snprintf(data, sizeof(data), "{\"id\":\"%s\",\"key\":\"%s\"}", device_id, device_key);

  free(device_id);
  free(device_key);

  char url[OTA_MAX_URL_LENGTH];
  snprintf(url, sizeof(url), "%s/v1/device/signup", ota_server);

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

  ESP_LOGD(TAG, "Signup request, status code: %d", status_code);

  if (status_code != 201) {
    ESP_LOGE(TAG, "Signup request, unexpected status code %d", status_code);
    esp_http_client_cleanup(client);
    return false;
  }

  esp_http_client_cleanup(client);

  return true;
}

void
ota_check () {
  char *current_hash = ota_nvs_read_string("ota", "hash");

  char url[OTA_MAX_URL_LENGTH];
  snprintf(url, sizeof(url), "%s/v1/check/%s?hash=%s", ota_server, ota_firmware_id, current_hash);

  free(current_hash);

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
    ESP_LOGE(TAG, "Check request, failed: %s", esp_err_to_name(err));
    goto done;
  }

  int status_code = esp_http_client_get_status_code(client);

  ESP_LOGD(TAG, "Check request, status code: %d", status_code);

  if (status_code == OTA_STATUS_NOT_FOUND) {
    ESP_LOGI(TAG, "Firmware not found");
    goto done;
  }

  if (status_code == OTA_STATUS_UP_TO_DATE) {
    ESP_LOGI(TAG, "Firmware is up to date");
    goto done;
  }

  if (status_code == OTA_STATUS_AVAILABLE) {
    char new_hash[64 + 1] = {0};
    int read_len = esp_http_client_read_response(client, new_hash, sizeof(new_hash) - 1);

    if (read_len < 0) {
      ESP_LOGE(TAG, "Check request, failed to read response");
      goto done;
    }

    new_hash[read_len] = '\0';

    snprintf(url, sizeof(url), "%s/v1/download/%s/%s", ota_server, ota_firmware_id, new_hash);

    esp_err_t err_download = ota_update(url);

    if (err_download != ESP_OK) {
      ESP_LOGE(TAG, "Check request, failed to update: %d", err_download);
      goto done;
    }

    ota_nvs_write_string("ota", "hash", new_hash);

    // TODO: Allow manual restart to avoid interruption, and timeout to force it
    esp_restart();

    goto done;
  }

  ESP_LOGE(TAG, "Check request, unexpected status code %d", status_code);

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
    .http_client_init_cb = http_client_init_cb,
  };

  // TODO: Use the advanced OTA APIs
  return esp_https_ota(&ota_config);
}

static esp_err_t
http_client_init_cb (esp_http_client_handle_t client) {
  if (ota_device_key != NULL) {
    esp_http_client_set_header(client, "x-ota-device", ota_device_key);
  }

  return ESP_OK;
}

static void
bytes_to_hex_fill (const uint8_t *buffer, size_t size, char *hex) {
  size_t length = size * 2;
  char *hex = malloc(length + 1);

  for (int i = 0; i < size; i++) {
    sprintf(&hex[i * 2], "%02x", buffer[i]);
  }

  hex[length] = '\0';

  return hex;
}

static char *
bytes_to_hex (const uint8_t *buffer, size_t size) {
  size_t length = size * 2;
  char *hex = malloc(length + 1);

  for (int i = 0; i < size; i++) {
    sprintf(&hex[i * 2], "%02x", buffer[i]);
  }

  hex[length] = '\0';

  return hex;
}
