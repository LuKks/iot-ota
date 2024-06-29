void
ota_log (char *message, ...) {
  if (!_ota_verbose) return;

  va_list argp;
  va_start(argp, message);

  vprintf(message, argp);
  printf("\n");

  va_end(argp);
}
