# ArduboyI2C Library
The **ArduboyI2C** library provides I2C support for Arduboy multiplayer games. It includes standard I2C functionality, support for multi-controller (multi-master) setups, and helpers for multiplayer handshakes while keeping PROGMEM and RAM usage low.

## Library Documentation
Full API documentation: https://sub1inear.github.io/ArduboyI2C/

## Features
- Controller (master) and target (slave) I2C support.
- Optional multi-controller safety checks for bus contention.
- Built-in multiplayer handshake with address assignment.
- Optional cable-flip detection for FX-C.
- Minimal memory overhead.

## Installation

### Arduino IDE
Install via the Library Manager:
1. Open the Arduino IDE.
2. Go to **Sketch > Include Library > Manage Libraries...**
3. Search for "ArduboyI2C" and install the library by sub1inear.

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
4. Use `I2C::write(address, data, length)` and `I2C::read(address, buffer, length)` for communication.
Find addresses from IDs using `I2C::getAddressFromId(id)`.
`I2C::write`s can use the general call address (`I2C_GENERAL_CALL`) to send data to all devices.
This is simpler than addressing a device individually.

```cpp
I2C::write(I2C_GENERAL_CALL, data, length);
I2C::read(I2C::getAddressFromId(id), buffer, length);
```

## Configuration
Define these before including `ArduboyI2C.h` to customize behavior:
- `I2C_FREQUENCY` (default 100000), increase for faster communication
- `I2C_BUFFER_SIZE` (default 32), max bytes per I2C transaction
- `I2C_CHECK_BUS_BUSY_CHECKS` (default 3), increase if the program freezes during reads/writes
- `I2C_CHECK_CABLE_FLIPPED_CHECKS` (default 128), increase for more sensitive cable flip detection
- `I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_TIMEOUT` (default 1000 ms), increase to allow more time for cable flip debounce
- `I2C_USE_HANDSHAKE` (default 1), set to 0 to disable the handshake function if using a custom one
- `I2C_USE_CHECK_BUS_BUSY` (default 1), set to 0 to disable bus busy checks
- `I2C_USE_CHECK_CABLE_FLIPPED` (default 1), set to 0 to disable cable flip detection for FX-C

## Differences from the Wire Library
- Highly optimized (saves ~2KiB of PROGMEM and ~200 bytes of RAM)
- Built-in handshake function for multiplayer games
- Multi-controller support for easy peer-to-peer communication
    - Optional bus busy checks to prevent freezes
- Built-in cable flip detection for FX-C
- Bufferless reading (unlike Wire)