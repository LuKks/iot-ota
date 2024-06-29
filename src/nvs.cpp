// TODO: Stop using ESP_ERROR_CHECK everywhere due abort()

void
ota_nvs_init () {
  esp_err_t err = nvs_flash_init();

  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  } else {
    ESP_ERROR_CHECK(err);
  }
}

void
ota_nvs_write_string (const char *namespace, const char *key, const char *value) {
  nvs_init();

  nvs_handle_t nvs;

  ESP_ERROR_CHECK(nvs_open(namespace, NVS_READWRITE, &nvs));
  ESP_ERROR_CHECK(nvs_set_str(nvs, key, value));
  ESP_ERROR_CHECK(nvs_commit(nvs));
  nvs_close(nvs);

  ESP_ERROR_CHECK(nvs_flash_deinit());
}

char *
ota_nvs_read_string (const char *namespace, const char *key) {
  nvs_init();

  nvs_handle_t nvs;

  ESP_ERROR_CHECK(nvs_open(namespace, NVS_READONLY, &nvs));

  size_t size;
  esp_err_t err = nvs_get_str(nvs, key, NULL, &size);

  if (err == ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(nvs);
    ESP_ERROR_CHECK(nvs_flash_deinit());
    return NULL;
  }

  ESP_ERROR_CHECK(err);

  char *value = malloc(size);
  ESP_ERROR_CHECK(nvs_get_str(nvs, key, value, &size));

  nvs_close(nvs);

  ESP_ERROR_CHECK(nvs_flash_deinit());

  return value;
}
