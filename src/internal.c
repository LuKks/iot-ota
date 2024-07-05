#include "internal.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

const char *TAG = "OTA_INTERNAL";

char *
bytes_to_hex (const uint8_t *buffer, size_t size) {
  size_t length = size * 2;
  char *hex = malloc(length + 1);

  for (size_t i = 0; i < size; i++) {
    sprintf(&hex[i * 2], "%02x", buffer[i]);
  }

  hex[length] = '\0';

  return hex;
}

int
pow_two (int n) {
  return 1 << n;
}

int
retry_delay (int delay, int max, int *retries) {
  return delay * pow_two(*retries >= max ? *retries : (*retries)++);
}

void
log_heap (const char *text) {
  ESP_LOGE(TAG, "Free heap: %d (%s)", (size_t) esp_get_free_heap_size(), text);
}

void
log_min_heap (const char *text) {
  ESP_LOGE(TAG, "Minimum free heap: %d (%s)", (size_t) esp_get_minimum_free_heap_size(), text);
}

void
log_min_stack (const char *text) {
  ESP_LOGE(TAG, "Minimum free stack: %d (%s)", (size_t) uxTaskGetStackHighWaterMark(NULL), text);
}
