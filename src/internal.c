#include "internal.h"

#include <stddef.h>
#include <stdint.h>

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
