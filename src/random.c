#include "random.h"

#include <esp_random.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void
random_bytes_fill (uint8_t *buffer, size_t size) {
  uint32_t value = 0;

  for (size_t i = 0; i < size; i++) {
    if (i % 4 == 0) {
      value = esp_random();
    }

    buffer[i] = value & 0xFF;

    value >>= 8;
  }
}

uint8_t *
random_bytes (size_t size) {
  uint8_t *buffer = malloc(size);

  random_bytes_fill(buffer, size);

  return buffer;
}
