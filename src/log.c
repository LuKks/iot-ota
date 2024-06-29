#include "log.h"

#include <ctype.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

bool _ota_verbose = false;

void
ota_log (char *message, ...) {
  if (!_ota_verbose) return;

  va_list args;
  va_start(args, message);

  vprintf(message, args);
  printf("\n");

  va_end(args);
}
