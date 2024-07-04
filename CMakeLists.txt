idf_component_register(
  SRC_DIRS "src"
  INCLUDE_DIRS "include"
  PRIV_REQUIRES esp_https_ota esp_partition nvs_flash
)
