idf_component_register(
  SRC_DIRS "src"
  INCLUDE_DIRS "include"
  PRIV_REQUIRES iot_crypto iot_nvs esp_https_ota esp_partition nvs_flash
)
