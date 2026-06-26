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
| `I2C_USE_CHECK_BUS_BUSY` | 1 | 0, 1 | Enable bus busy checking functionality |
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
