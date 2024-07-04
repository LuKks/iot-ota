# ota

Firmware OTA updates library in C for IoT devices

## Usage

Depends on:

- [ota-backend](https://github.com/lukks/ota-backend) for hosting firmwares.
- [ota-update](https://github.com/lukks/ota-update) to interact with the backend.

```c
#include <WiFi.h>
#include <ota.h>

#define FIRMWARE_ID "<firmware id>"

void
setup () {
  Serial.begin(115200);

  WiFi.begin("<wifi ssid>", "<wifi pass>");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting");
    delay(500);
  }

  ota_updates(FIRMWARE_ID);
}

void
loop () {
  Serial.println("loop()");
  delay(999999);
}
```

## API

See [`include/ota.h`](include/ota.h) for the public API.

#### `ota_set_server(url)`

Set a custom server, in case you self-host your own.

By default, it uses https://ota.leet.ar with no guarantees for now.

#### `ota_updates(firmware_id)`

Run updates on background. It also generates and saves a random device ID.

When you pass a firmware id, it always gets saved in the NVS.

Later you can pass NULL, and it will reuse the one already saved.

## License

MIT
