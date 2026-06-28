[![Release](https://img.shields.io/github/v/release/sub1inear/ArduboyI2C?label=Release)](https://github.com/sub1inear/ArduboyI2C/releases/latest)
[![License](https://img.shields.io/github/license/sub1inear/ArduboyI2C?label=License)](https://github.com/sub1inear/ArduboyI2C/blob/main/LICENSE)
[![Arduino Library](https://img.shields.io/badge/Arduino-Library-00979D?logo=arduino)](https://www.arduinolibraries.info/libraries/arduboy-i2-c)
[![PlatformIO Registry](https://img.shields.io/badge/PlatformIO-Registry-orange?logo=platformio)](https://registry.platformio.org/libraries/sub1inear/ArduboyI2C)
[![Docs](https://img.shields.io/badge/Docs-Online-blue)](https://sub1inear.github.io/ArduboyI2C/)

# ArduboyI2C Library
The **ArduboyI2C** library provides I2C support for Arduboy multiplayer games. It includes standard I2C functionality, support for multi-controller (master) setups, and helpers for multiplayer handshakes while keeping PROGMEM and RAM usage low.

## Documentation
See the [online documentation](https://sub1inear.github.io/ArduboyI2C/) for more information.

## Features
- Controller (master) and target (slave) I2C support.
- Built-in handshake function for multiplayer games.
- Multi-controller support for easy peer-to-peer communication.
    - Optional bus busy checks to prevent freezes.
- Built-in cable flip detection for FX-C.
- Bufferless reading.
- Minimal memory overhead.
- Customizable configuration.

## Why ArduboyI2C over Wire?
- Highly optimized (saves ~2KiB of PROGMEM and ~200 bytes of RAM).
- Adds functions for multiplayer games (handshake, multi-controller support).
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
I2C::write(I2C_GENERAL_CALL, data, /* wait = */true);
I2C::read(I2C::idtoAddress(id), data);
```
> Write data to every device with the `I2C_GENERAL_CALL` address.
### 5. Callbacks
```cpp
I2C::onReceive(receiveCallback);
I2C::onRequest(requestCallback);
```

```cpp
void requestCallback() {
  I2C::reply(data);
}
```

## Configuration
Define before `#include <ArduboyI2C.h>`.
### Core Settings
| Macro | Default | Options | Description |
|-------|---------|---------|-------------|
| `I2C_PLATFORM` | `I2C_PLATFORM_FX_C` | `I2C_PLATFORM_MINI`, `I2C_PLATFORM_FX_C`, `I2C_PLATFORM_UNKNOWN` | Platform presets |
| `I2C_FREQUENCY` | 100000 | 0-400000 | Bus frequency (Hz) |
| `I2C_BUFFER_SIZE` | 32 | 1-255 | Transaction buffer size |
### Reliability
| Macro | Default | Options | Description |
|-------|---------|---------|-------------|
| `I2C_CHECK_BUS_BUSY_CHECKS` | 16 | 1-255 | Number of bus busy checks before read/write |
| `I2C_CHECK_CABLE_FLIPPED_CHECKS` | 128 | 1-255 | Number of checks for flipped cable detection |
| `I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_TIMEOUT` | 1000 | 1-32767 | Debounce time (ms) for cable flip detection |
### Feature Flags
| Macro | Default | Options | Description |
|-------|---------|---------|-------------|
| `I2C_USE_HANDSHAKE` | 1 | 0, 1 | Enable handshake functionality |
| `I2C_USE_MULTI_CONTROLLER` | 1 | 0, 1 | Enable multi-controller safety checks |
| `I2C_USE_CHECK_CABLE_FLIPPED` | 1 (FX-C), 0 (Mini), None (Unknown) | 0, 1 | Enable flipped cable detection functionality |
| `I2C_USE_SOFTWARE_PULLUPS` | 1 (FX-C), 0 (Mini), None (Unknown) | 0, 1 | Enable software pullups on SDA/SCL lines |

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

### My game sometimes freezes, what causes this?
In multi-controller setups, two devices may try to control the bus simultaneously. Increase `I2C_MULTI_CONTROLLER_BUSY_CHECKS` (default: 16, max: 255) to add more pre-flight bus checks. Check `I2C::getError()` after writes/reads for `I2C_ERROR_ARB_LOST` and retry the operation. If this does not fix it or you're not in a multi-controller setup, please open an issue [here](https://github.com/sub1inear/ArduboyI2C/issues/new).

### Can I use ArduboyI2C in Ardens?
[Ardens](https://github.com/tiberiusbrown/Ardens) currently doesn't simulate I2C hardware. Use `I2C::checkEmulator()` to detect this and print a message.

### What does `I2C_HANDSHAKE_FULL` mean?
It means the bus already has `numPlayers` devices connected, so there's no slot left for this device. You can print this message or just call `arduboy.exitToBootloader()`.

### When should I use `I2C::write()` vs `I2C::reply()`?
Use `I2C::write()` as the controller (master) to send data. Use `I2C::reply()` inside an `onRequest` callback to send data back when another device reads from you. Calling `write()` from a callback won't work correctly.

### How do I send/receive more than 32 bytes at once?
Writes and `reply()` are limited by `I2C_BUFFER_CAPACITY` (default: 32). Define it before including the header (e.g., `#define I2C_BUFFER_CAPACITY 64`). Reads have no such buffer limit.

### Why must `I2C::begin()` be called after `arduboy.begin()`?
`arduboy.boot()` (called inside `arduboy.begin()`) disables the TWI (I2C) hardware as a side effect. Calling `I2C::begin()` afterwards re-initializes it.

### Why can't I use certain I2C addresses for my own address?
Addresses 0–7 and 120–127 are reserved by the I2C spec (general call, broadcast, clock extension, etc.). `I2C::idToAddress()` maps IDs 0–111 to safe addresses 8–119.

### Is `checkCableFlipped()` 100% reliable?
No, it works by sampling which line looks more like a clock signal. Edge cases (very short cables, noisy lines, both devices idle) can cause false positives. Increase `I2C_CHECK_CABLE_FLIPPED_CHECKS` for more accuracy at the cost of a longer detection time.

### How do I minimize memory usage?
Disable unused features (`I2C_USE_HANDSHAKE` and `I2C_USE_CHECK_CABLE_FLIPPED`) and reduce `I2C_BUFFER_CAPACITY` to the smallest value that meets your needs. Always use `I2C_GENERAL_CALL_ADDR` and have every device be a controller/master. The [Configuration](#configuration) table lists all available toggles.
