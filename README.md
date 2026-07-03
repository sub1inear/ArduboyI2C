[![Release](https://img.shields.io/github/v/release/sub1inear/ArduboyI2C?label=Release)](https://github.com/sub1inear/ArduboyI2C/releases/latest)
[![License](https://img.shields.io/github/license/sub1inear/ArduboyI2C?label=License)](https://github.com/sub1inear/ArduboyI2C/blob/main/LICENSE)
[![Arduino Library](https://img.shields.io/badge/Arduino-Library-00979D?logo=arduino)](https://www.arduinolibraries.info/libraries/arduboy-i2-c)
[![PlatformIO Registry](https://img.shields.io/badge/PlatformIO-Registry-orange?logo=platformio)](https://registry.platformio.org/libraries/sub1inear/ArduboyI2C)
[![Docs](https://img.shields.io/badge/Docs-Online-blue)](https://sub1inear.github.io/ArduboyI2C/)

# ArduboyI2C Library
The **ArduboyI2C** library provides I2C support for Arduboy multiplayer games. It includes standard I2C functionality, support for multi-controller (master) setups, and helpers for multiplayer handshakes while keeping PROGMEM and RAM usage low.

## Documentation
See the [online documentation](https://sub1inear.github.io/ArduboyI2C/) for an API reference.
See [An I2C Primer](./docs/AnI2CPrimer.md) for a more in-depth explanation of writing I2C games.

## Features
- Controller (master) and target (slave) I2C support.
- Built-in handshake function for multiplayer games.
- Built-in cable flip detection for FX-C.
- Bufferless reading.
- Minimal memory overhead.
- Customizable configuration.

## Why ArduboyI2C over Wire?
- Highly optimized (saves ~1KiB of PROGMEM and ~200 bytes of RAM).
- Adds easy handshake functionality.
- Adds cable flip detection for FX-C.
- More customizable (see [Configuration](#configuration)).

## Installation

### Arduino IDE
Install via the Library Manager:
1. Open the Arduino IDE.
2. Go to **Sketch > Include Library > Manage Libraries...**
3. Search for "ArduboyI2C" and install the library by `sub1inear`.

### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps =
	sub1inear/ArduboyI2C
```

## Quick Start
### 1. Include the library
```cpp
#define I2C_IMPLEMENTATION
#include <ArduboyI2C.h>
```
### 2. Initialize
```cpp
void setup() {
    arduboy.begin();
    I2C::begin();
    ...
}
```
### 3. Handshake
```cpp
uint8_t id = I2C::handshake(numPlayers);
```
### 4. Read/Write
```cpp
I2C::write(I2C_TARGET_ADDRESS, data, /* wait = */true);
I2C::read(I2C_TARGET_ADDRESS, data);
```
### 5. Callbacks
```cpp
I2C::onReceive(onReceive);
I2C::onRequest(onRequest);
```

```cpp
void onReceive() {
    const uint8_t *data = I2C::getBuffer();
    ...
}
void onRequest() {
    I2C::reply(data);
}
```

## Configuration
Define before `#include <ArduboyI2C.h>`.
| Macro | Default | Options | Description |
|-------|---------|---------|-------------|
| `I2C_IMPLEMENTATION` | - | - | Must be defined in **one** source file to include the implementation. |
| `I2C_FREQUENCY` | 100000 | 0-400000 | Bus frequency (Hz) |
| `I2C_BUFFER_SIZE` | 32 | 1-255 | Transaction buffer size |

## Migrating from Wire Library
| Wire API               | ArduboyI2C              |
| ---------------------- | ----------------------- |
| `Wire.begin()`         | `I2C::begin()`          |
| `Wire.begin(addr)`     | `I2C::setAddress(addr)` |
| `Wire.onReceive()`     | `I2C::onReceive()`      |
| `Wire.onRequest()`     | `I2C::onRequest()`      |
| `Wire.beginTransmission(addr)` -> `Wire.write(...)` -> `Wire.endTransmission()` | `I2C::write(address, data, wait)` |
| `Wire.requestFrom(address, quantity)` -> `Wire.read()` | `I2C::read(address, data, wait)` |
| `Wire.setClock()`      | `I2C_FREQUENCY`         |

## FAQ
### Why am I getting linker errors about ArduboyI2C functions?
First, make sure you define `I2C_IMPLEMENTATION` in **one** source file before including `ArduboyI2C.h`.
If this doesn't fix your issue, make sure you haven't disabled any features that you are using (e.g. disabling `I2C_USE_HANDSHAKE` will remove the `I2C::handshake()` function, disabling `I2C_USE_CHECK_CABLE_FLIPPED` or defining `I2C_PLATFORM` as `I2C_PLATFORM_MINI` will remove the `I2C::checkCableFlipped()` function, etc.).

### Why can't I use `delay()`, `Serial`, `millis()`, etc. inside `onReceive`/`onRequest` callbacks?
Callbacks are executed inside the I2C interrupt handler, where interrupts are disabled. Functions that rely on interrupts (`delay`, `millis`, `Serial`, etc.) won't work. Keep callbacks short and only use `I2C::reply()` and `I2C::getBuffer()`. If you need to do extended processing or rely on interrupts, set a `volatile bool` flag, copy the data out of the buffer, and then poll the flag in your main loop.

### Can I use ArduboyI2C in Ardens?
[Ardens](https://github.com/tiberiusbrown/Ardens) will soon support I2C multiplayer; stay tuned.

### When should I use `I2C::write()` vs `I2C::reply()`?
Use `I2C::write()` as the controller (master) to send data. Use `I2C::reply()` inside an `onRequest` callback to send data back when another device reads from you. Calling `write()` from a callback won't work correctly.

### How do I send/receive more than 32 bytes at once?
Writes and `reply()` are limited by `I2C_BUFFER_CAPACITY` (default: 32). Define it before including the header (e.g., `#define I2C_BUFFER_CAPACITY 64`). Reads have no such buffer limit.

### Why must `I2C::begin()` be called after `arduboy.begin()`?
`arduboy.boot()` (called inside `arduboy.begin()`) disables the TWI (I2C) hardware as a side effect. Calling `I2C::begin()` afterwards re-initializes it.

### Why can't I use certain I2C addresses for my own address?
Addresses 0–7 and 120–127 are reserved by the I2C spec (general call, etc.).
