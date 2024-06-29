#include "log.h"

#include "../include/ota.h"

#include <stdarg.h>
#include <stdio.h>

extern int ota_verbose;

void
ota_log (char *message, ...) {
  if (!ota_verbose) return;

  va_list args;
  va_start(args, message);

  vprintf(message, args);
  printf("\n");

  va_end(args);
}
