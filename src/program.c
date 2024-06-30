#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static void
generate_random_bytes (uint8_t *buffer, size_t length);

static char *
bytes_to_hex (uint8_t *bytes, size_t size);

static void
seed_random ();

int
main () {
  seed_random();

  uint8_t id[32];

  printf("%d\n", id[0]);
  printf("%d\n", id[31]);

  printf("\n");

  generate_random_bytes(id, sizeof(id));

  printf("%d\n", id[0]);
  printf("%d\n", id[31]);

  printf("\n");

  char *hex = bytes_to_hex(id, sizeof(id));

  printf("%s\n", hex);

  free(hex);

  printf("%s\n", hex);

  return 0;
}

static void
generate_random_bytes (uint8_t *buffer, size_t length) {
  for (size_t i = 0; i < length; i++) {
    buffer[i] = (uint8_t) (rand() & 0xFF);
  }
}

static char *
bytes_to_hex (uint8_t *bytes, size_t size) {
  int length = size * 2;
  char *hex = malloc(length + 1);

  for (int i = 0; i < size; i++) {
    sprintf(&hex[i * 2], "%02x", bytes[i]);
  }

  hex[length] = '\0';

  return hex;
}

static void
seed_random () {
  unsigned int seed = time(NULL) ^ getpid() ^ (uintptr_t) &seed;
  srand(seed);
}
