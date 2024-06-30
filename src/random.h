#ifndef OTA_RANDOM_H
#define OTA_RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void
random_bytes_fill (uint8_t *buffer, size_t size);

uint8_t *
random_bytes (size_t size);

#ifdef __cplusplus
}
#endif

#endif // OTA_RANDOM_H
