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
pow_two (int n) {
  return 1 << n;
}

int
retry_delay (int delay, int max, int *retries) {
  return delay * pow_two(*retries >= max ? *retries : (*retries)++);
}

void
change_string (char *str) {
  str[0] = 'a';
  str[1] = 'b';
  str[2] = 'c';
}

void
random_bytes_fill (uint8_t *buffer, size_t size) {
  printf("random_bytes_fill(%d)\n", (int)size);

  for (size_t i = 0; i < size; i++) {
    buffer[i] = (uint8_t) (rand() & 0xFF);
    printf("%02x", buffer[i]);
  }

  printf("\n");
}

uint8_t *
random_bytes (size_t size) {
  printf("random_bytes(%d)\n", (int)size);

  uint8_t *buffer = malloc(size);

  printf("buffer %d\n", (int)sizeof(buffer));

  random_bytes_fill(buffer, size);

  return buffer;
}

void
print_hex (const uint8_t *data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    printf("%02x", data[i]);
  }
  printf("\n");
}

char name[] = "luk";

void
log (const char str[]) {
  name = str;

  printf("%s\n", name);
}

int
main () {
  seed_random();

  log("lukks");

  uint8_t *id = random_bytes(4);

  print_hex(id, sizeof(id));

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
