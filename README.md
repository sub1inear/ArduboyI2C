# ArduboyI2C Library
The **ArduboyI2C** library provides I2C support for Arduboy multiplayer games. It includes standard I2C functionality, support for multi-controller (master) setups, and helpers for multiplayer handshakes while keeping PROGMEM and RAM usage low.

## Library Documentation
Full API documentation: https://sub1inear.github.io/ArduboyI2C/

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
3. Search for "ArduboyI2C" and install the library by "sub1inear".

### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps =
	sub1inear/ArduboyI2C
```

## Quick Start
1. Include the library:
```cpp
#include <ArduboyI2C.h>
```
2. Initialize the I2C bus in `setup()`:
```cpp
void setup() {
  I2C::begin();
}
```
3. Use `I2C::handshake(numPlayers)` to perform a multiplayer handshake and get a unique ID for each controller:
```cpp
uint8_t id = I2C::handshake(numPlayers);
```
4. Use `I2C::write(address, data, wait)` and `I2C::read(address, data, wait)` for communication.
Find addresses from IDs using `I2C::idtoAddress(id)`.
`I2C::write`s can use the general call address (`I2C_GENERAL_CALL`) to send data to all devices.
This is simpler than addressing a device individually, even if only using two players.
If multiple devices are sending/receiving data, this is known as a multi-controller/master setup.
```cpp
I2C::write(I2C_GENERAL_CALL, data, true);
I2C::read(I2C::idtoAddress(id), data, true);
```

5. Register callbacks for when data is received (from an `I2C::write`) or requested (from an `I2C::read`):
```cpp
I2C::onReceive(receiveCallback);
I2C::onRequest(requestCallback);
```
Call `I2C::reply` to send data back.
```cpp
void requestCallback() {
  I2C::reply(data, length);
}
```

## Configuration
Define these before including `ArduboyI2C.h` to customize behavior:
- `I2C_FREQUENCY` (default 100000), increase for faster communication
- `I2C_BUFFER_SIZE` (default 32), max bytes per I2C transaction
- `I2C_CHECK_BUS_BUSY_CHECKS` (default 16), increase if the program freezes during reads/writes
- `I2C_CHECK_CABLE_FLIPPED_CHECKS` (default 128), increase for more sensitive cable flip detection
- `I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_TIMEOUT` (default 1000 ms), increase to allow more time for cable flip debounce
- `I2C_USE_HANDSHAKE` (default 1), set to 0 to disable the handshake function if using a custom one
- `I2C_USE_CHECK_BUS_BUSY` (default 1), set to 0 to disable bus busy checks
- `I2C_USE_CHECK_CABLE_FLIPPED` (default 1), set to 0 to disable cable flip detection for FX-C

## Migrating from Wire Library
- `Wire.begin()`
    - `I2C::begin()`
    - No need to call `power_twi_enable()`; it is done automatically.
- Manual handshaking
    - `I2C::handshake(numPlayers)`
- `Wire.begin(address)`
    - `I2C::setAddress(address)`
- `Wire.onReceive(receiveCallback)`
    - `I2C::onReceive(receiveCallback)`
- `Wire.onRequest(requestCallback)`
    - `I2C::onRequest(requestCallback)`
- `Wire.beginTransmission(address)` -> `Wire.write(...)` -> `Wire.endTransmission()`
    - `I2C::write(address, data, wait)`
    - Must put data in a buffer/`struct` first in RAM
- `Wire.write(data)` (inside `onRequest` callback)
    - `I2C::reply(data)`
    - May be called multiple times to accumulate data
    - Data is sent when the callback finishes
- `Wire.requestFrom(address, quantity)` -> `Wire.read()`
    - `I2C::read(address, data, wait)`
    - Reads directly into a buffer/`struct` in RAM, no need to call `Wire.read()` multiple times.
- `Wire.setClock(frequency)`
    - `I2C_FREQUENCY` configuration option.
- `Wire.setWireTimeout(timeout)`
    - No need; `ArduboyI2C` should never freeze.
    - If freezing ever occurs during multi-controller use, increase `I2C_CHECK_BUS_BUSY_CHECKS`.
- ArduboyI2C has no distinction between controller (master) and target (slave) modes.
    - Every device can send and receive data at the same time (multi-controller/master).