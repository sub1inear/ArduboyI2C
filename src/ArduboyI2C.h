/*
MIT License

Copyright (c) 2024-2026 sub1inear

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
/** @file
 * \brief
 * An I2C library for Arduboy multiplayer games.
 */
#pragma once
#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/twi.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

/** \brief
 * Constant for the Arduboy FX-C platform.
 * \details
 * Set `I2C_PLATFORM` to this value to use the Arduboy FX-C platform.
 * This is the default platform if `I2C_PLATFORM` is not defined.
 */
#define I2C_PLATFORM_FX_C 0

/** \brief
 * Constant for the Arduboy Mini platform.
 * \details
 * Set `I2C_PLATFORM` to this value to use the Arduboy Mini platform.
 */
#define I2C_PLATFORM_MINI 1

/** \brief
 * Constant for an unknown platform.
 * \details
 * Set `I2C_PLATFORM` to this value to use an unknown platform.
 */
#define I2C_PLATFORM_UNKNOWN 2

#ifdef __DOXYGEN__
/** \brief
 * The platform being used.
 * \details
 * No default.
 * Options: \n
 * - `I2C_PLATFORM_FX_C`: Arduboy FX-C \n
 * - `I2C_PLATFORM_MINI`: Arduboy Mini \n
 * - `I2C_PLATFORM_UNKNOWN`: Unknown platform (must define all other platform-specific macros)
 */
#define I2C_PLATFORM
#elif defined(I2C_IMPLEMENTATION) && !defined(I2C_PLATFORM)
#error "I2C_PLATFORM must be defined."
#endif

#ifndef I2C_FREQUENCY
/** \brief
 * The initial I2C frequency in Hz.
 * \details
 * Defaults to 100000 Hz, with a maximum of 400000 Hz. \n
 * Standard frequencies: \n
 * 100000 Hz - Standard Mode \n
 * 400000 Hz - Fast Mode
 */
#define I2C_FREQUENCY 100000
#elif I2C_FREQUENCY > 400000
#error "I2C_FREQUENCY is too high."
#endif

#ifndef I2C_BUFFER_CAPACITY
/** \brief
 * The capacity of the buffer used for writes/target (slave) operations.
 * \details
 * Defaults to 32. If more than 32 bytes are needed for writes/target (slave) operations, increase. If more RAM is needed, decrease.
 * Maximum is 255.
 */
#define I2C_BUFFER_CAPACITY 32
#elif I2C_BUFFER_CAPACITY > 255
#error "I2C_BUFFER_CAPACITY is too big."
#endif

#ifndef I2C_MULTI_CONTROLLER_BUSY_CHECKS
/** \brief
 * The amount of times the bus is checked before continuing with a read/write operation.
 * \details
 * Defaults to 16, with a maximum of 255. Fixes design flaw where TWI hardware does not check if the bus has become busy during a stop interrupt,
 * so if multiple targets (slaves) receive the stop interrupt right before
 * they become the controller (master) and send a start, they all will think the bus is free and clobber each other.
 * Increase if the game ever freezes.
 * More information: https://www.robotroom.com/Atmel-AVR-TWI-I2C-Multi-Master-Problem.html
 */
#define I2C_MULTI_CONTROLLER_BUSY_CHECKS 8
#elif I2C_MULTI_CONTROLLER_BUSY_CHECKS > 255
#error "I2C_MULTI_CONTROLLER_BUSY_CHECKS is too big."
#endif

#ifndef I2C_CHECK_CABLE_FLIPPED_CHECKS
/** \brief
 * The total number of checks to perform when checking for a flipped cable.
 * \details
 * Defaults to 128, with a maximum of 255. Increase for a more accurate detection at the cost of a longer detection time.
 */
#define I2C_CHECK_CABLE_FLIPPED_CHECKS 128
#elif I2C_CHECK_CABLE_FLIPPED_CHECKS > 255
#error "I2C_CHECK_CABLE_FLIPPED_CHECKS is too big."
#endif

#ifndef I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_TIMEOUT
/** \brief
 * The duration in milliseconds to debounce cable changes when checking for a flipped cable.
 * \details
 * Defaults to 1000 ms, with a maximum of 32767 ms. Increase for more stable detection at the cost of a longer wait time when flipping the cable.
 */
#define I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_TIMEOUT 1000
#elif I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_TIMEOUT > 32767
#error "I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_TIMEOUT is too big."
#endif

#ifndef I2C_USE_HANDSHAKE
/** \brief
 * Whether or not to enable handshake functionality.
 * \details
 * Defaults to 1.
 * Set to 0 if you do not use the built-in handshake (e.g. you use a custom handshake or no handshake at all) to save memory.
 */
#define I2C_USE_HANDSHAKE 1
#endif

#ifndef I2C_USE_MULTI_CONTROLLER
/** \brief
 * Whether or not to enable multi-controller (master) safety checks.
 * \details
 * Defaults to 1.
 * Set to 0 if there is only one controller (master) on the bus to save memory.
 */
#define I2C_USE_MULTI_CONTROLLER 1
#endif

#ifndef I2C_USE_CHECK_CABLE_FLIPPED
/** \brief
 * Whether or not to enable flipped cable detection functionality.
 * \details
 * Default is 1 on the FX-C and 0 on the Mini.
 * Set to 0 if you do not need to detect flipped cables to save memory.
 */
#if I2C_PLATFORM == I2C_PLATFORM_FX_C
#define I2C_USE_CHECK_CABLE_FLIPPED 1
#elif I2C_PLATFORM == I2C_PLATFORM_MINI
#define I2C_USE_CHECK_CABLE_FLIPPED 0
#else
#error "I2C_USE_CHECK_CABLE_FLIPPED must be defined for unknown platforms."
#endif
#endif

#if !I2C_USE_MULTI_CONTROLLER && I2C_USE_CHECK_CABLE_FLIPPED
#error "I2C_USE_CHECK_CABLE_FLIPPED cannot be enabled without I2C_USE_MULTI_CONTROLLER."
#endif

#ifndef I2C_USE_SOFTWARE_PULLUPS
/** \brief
 * Whether or not to enable software pullups on the SDA and SCL lines.
 * \details
 * Default is 1 on the FX-C and 0 on the Mini.
 * Set to 0 if you have external pullups on the SDA and SCL lines to save memory.
 */
#if I2C_PLATFORM == I2C_PLATFORM_FX_C
#define I2C_USE_SOFTWARE_PULLUPS 1
#elif I2C_PLATFORM == I2C_PLATFORM_MINI
#define I2C_USE_SOFTWARE_PULLUPS 0
#else
#error "I2C_USE_SOFTWARE_PULLUPS must be defined for unknown platforms."
#endif
#endif

#ifndef I2C_SDA_BIT
/** \brief
 * The bit of the pin on which the SDA line is connected.
 * \details
 * Defaults to PIND1.
 */
#define I2C_SDA_BIT PIND1
#endif

#ifndef I2C_SCL_BIT
/** \brief
 * The bit of the pin on which the SCL line is connected.
 * \details
 * Defaults to PIND0.
 */
#define I2C_SCL_BIT PIND0
#endif

#ifndef I2C_PIN
/** \brief
 * The pin on which the SDA and SCL lines are connected.
 * \details
 * Defaults to PIND.
 * \note
 * There can be only one pin for the SDA and SCL lines to increase optimization.
 */
#define I2C_PIN PIND
#endif

#ifndef I2C_PORT
/** \brief
 * The port on which the SDA and SCL lines are connected.
 * \details
 * Defaults to PORTD.
 * \note
 * There can be only one port for the SDA and SCL lines to increase optimization.
 */
#define I2C_PORT PORTD
#endif

#ifndef I2C_DDR
/** \brief
 * The data direction register for the SDA and SCL lines.
 * \details
 * Defaults to DDRD.
 * \note
 * There can be only one data direction register for the SDA and SCL lines to increase optimization.
 */
#define I2C_DDR DDRD
#endif

/** \brief
 * The address used for general calls.
 * \details
 * General calls are sent to this address and are received by every device on the bus.
 */
#define I2C_GENERAL_CALL_ADDR 0x00

/** \brief
 * Target (slave) did not acknowledge its address during a write; it is not connected or is not responding.
 * \details
 * Error code returned by I2C::getError().
 */
#define I2C_ERROR_WRITE_ADDR_NACK TW_MT_SLA_NACK

/** \brief
 * Target (slave) did not acknowledge the data sent to it during a write; it disconnected or hung in the middle of receiving.
 * \details
 * Error code returned by I2C::getError().
 */
#define I2C_ERROR_WRITE_DATA_NACK TW_MT_DATA_NACK

/** \brief
 * Target (slave) did not acknowledge its address during a read; it is not connected or is not responding.
 * \details
 * Error code returned by I2C::getError().
 */
#define I2C_ERROR_READ_ADDR_NACK TW_MR_SLA_NACK

/** \brief
 * This device lost arbitration while trying to become the controller (master); another device is already the controller (master).
 * There code has no distinction between reads and writes (hardware constraint).
 * \details
 * Error code returned by I2C::getError().
 */
#define I2C_ERROR_ARB_LOST TW_MT_ARB_LOST

/** \brief
 * An illegal start or stop condition was detected on the bus.
 * \details
 * Error code returned by I2C::getError().
 */
#define I2C_ERROR_BUS TW_BUS_ERROR

/** \brief
 * No error has occurred.
 */
#define I2C_ERROR_NONE 0xFF

/** \brief
 * Error code returned by I2C::handshake, meaning a handshake has already been completed by the number of players specified.
 */
#define I2C_HANDSHAKE_FULL 0xFE

/** \brief
 * The maximum amount of ids available to a device.
 * \details
 * I2C uses a 7-bit addressing scheme with 128 available unique addresses.
 * However, addresses 0-7 and 120-127 are reserved by the standard and should not be used.
 * This leaves 112 unique addresses, and by extension, ids, for a device to use.
 */
#define I2C_MAX_IDS 112

/** \brief
 * I2C library major version.
 */
#define I2C_VERSION_MAJOR 3

/** \brief
 * I2C library minor version.
 */
#define I2C_VERSION_MINOR 0

/** \brief
 * I2C library patch version.
 */
#define I2C_VERSION_PATCH 0

/** \brief
 * I2C library version.
 * \details
 * For a given version x.y.z, the library version will be in the form x * 10000 + y * 100 + z.
 */
#define I2C_VERSION (I2C_VERSION_MAJOR * 10000 + I2C_VERSION_MINOR * 100 + I2C_VERSION_PATCH)

/// \cond
/** \brief
 * Not officially part of the library.
 */
namespace i2c_detail {
// minimal <type_traits> implementation (non-existent on avr-gcc)
template <typename T> struct is_pointer { static const bool value = false; };
template <typename T> struct is_pointer<T *> { static const bool value = true;  };
}
/// \endcond

/**
 * Provides all I2C functionality.
 */
class I2C {
public:
    /** \brief
     * Initializes I2C hardware.
     * \details
     * This function powers on, initializes, and sets up the clock on the TWI hardware.
     * Must be called after Arduboy hardware is initialized as `arduboy.boot()` (inside `arduboy.begin()`) disables the I2C (TWI) hardware.
     * \see end()
     */
    static void begin();

    /** \brief
     * Deinitializes I2C hardware.
     * \details
     * This function powers off and deinitializes the TWI hardware.
     * It may be reinitialized by calling `I2C::begin()` again.
     * \see begin()
     */
    static void end();

    /** \brief
     * Set the address of the device and whether to enable or disable general calls for it.
     * \param address The 7-bit address to respond to.
     * Addresses 0-7 and 120-127 are reserved by the standard and should not be used.
     * \param generalCall Whether to enable or disable general calls. Defaults to true.
     * \note
     * General calls are a way for a device to broadcast data to every other device without addressing them individually.
     * They are sent by sending a write to address `I2C_GENERAL_CALL_ADDR`. If they are disabled, the device will not respond to them.
     * These two functionalities are combined for efficiency, as together they make up the TWAR register.
     */
    static void setAddress(uint8_t address, bool generalCall = true);

    /** \brief
     * Attempts to become the bus controller (master) and sends data over I2C to the specified address.
     * \param address The 7-bit address to send the data to. To send a general call, use `I2C_GENERAL_CALL_ADDR`.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \param buffer A pointer to the data to send.
     * \param size The amount of data in bytes to send. This cannot be zero.
     * \param wait Whether or not to wait for the write to complete. If this is false, it will proceed with interrupts.
     * \details
     * \note
     * Sending general calls will only function if the `generalCall` argument of `setAddress` is true on other devices.
     * \note
     * Internally, this function uses a buffer to enable asynchronous writes. The buffer size is controlled by the macro `I2C_BUFFER_CAPACITY`
     * and defaults to 32. If the program needs to send more than 32 bytes at a time, `I2C_BUFFER_CAPACITY`
     * must be defined before including to be larger.
     * \see transmit() read()
     */
    static void write(uint8_t address, const void *buffer, uint8_t size, bool wait);

    /** \overload
     * \tparam T The type of the object to send. To prevent bugs, T cannot be a pointer.
     * \param address The 7-bit address to receive the data from.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \param object A reference to the object to send.
     * \param wait Whether or not to wait for the write to complete. If this is false, it will proceed with interrupts.
     * \details
     * This function will automatically deduce the size of the object.
     * Objects with sizes greater than or equal to 255 should not be used with this function.
     */
    template<typename T>
    static void write(uint8_t address, const T &object, bool wait) {
        static_assert(!i2c_detail::is_pointer<T>::value, "T cannot be a pointer.");
        static_assert(sizeof(T) <= I2C_BUFFER_CAPACITY, "Size of T must be less than or equal to I2C_BUFFER_CAPACITY.");
        I2C::write(address, (const void *)&object, sizeof(T), wait);
    }

    /** \brief
     * Attempts to become the bus controller (master) and reads data over I2C from the specified address.
     * \param address The 7-bit address to receive the data from.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \param buffer A pointer to the buffer in which to store the data.
     * \param size The maximum amount of bytes to receive. This cannot be 0 or 255.
     * \details
     * \note
     * Unlike the `write` function, this function is bufferless and is not limited to 32 bytes.
     * \see write()
     */
    static void read(uint8_t address, void *buffer, uint8_t size);

    /** \overload
     * \tparam T The type of the object to read. To prevent bugs, T cannot be a pointer.
     * \param address The 7-bit address to receive the data from.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \param object A reference to the object in which to store the data.
     * \details
     * This function will automatically deduce the size of the object.
     * Objects with sizes greater than or equal to 255 should not be used with this function.
     */
    template<typename T>
    static void read(uint8_t address, T &object) {
        static_assert(!i2c_detail::is_pointer<T>::value, "T cannot be a pointer.");
        static_assert(sizeof(T) < 255, "Size of T must be less than 255.");
        I2C::read(address, (void *)&object, sizeof(T));
    }

    /** \brief
     * Replies back to the controller (master).
     * \param buffer A pointer to the data to reply with.
     * \param size The amount of the data in bytes to reply with.
     * \details
     * This function is intended to be called inside the `onRequest` callback.
     * It fills the I2C buffer with data to then be sent one byte at a time.
     * If it is called more than once, only the last call will be sent.
     * \note
     * Internally, this function uses a buffer. The buffer size is controlled by the macro `I2C_BUFFER_CAPACITY`
     * and defaults to 32. If the program needs to send more than 32 bytes at a time, `I2C_BUFFER_CAPACITY`
     * must be defined before including to be larger.
     * \see write() onRequest()
     */
    static void reply(const void *buffer, uint8_t size);

    /** \overload
     * \tparam T The type of the object to reply with. To prevent bugs, `T` cannot be a pointer.
     * \param object A reference to the object to reply with.
     * \details
     * This function will automatically deduce the size of the object.
     * Objects with sizes greater than or equal to 255 should not be used with this function.
     */
    template <typename T>
    static void reply(const T &object) {
        static_assert(!i2c_detail::is_pointer<T>::value, "T cannot be a pointer.");
        static_assert(sizeof(T) <= I2C_BUFFER_CAPACITY, "Size of T must be less than or equal to I2C_BUFFER_CAPACITY.");
        I2C::reply((const void *)&object, sizeof(T));
    }

    /** \brief
     * Sets up/disables the callback to be called when data is requested from the device's address (a read).
     * \param function The function to be called when data is requested, or nullptr to disable.
     * \details
     * Example Callback and Usage:
     * \code{.cpp}
     * void dataRequest() {
     *   I2C::reply(players[id]);
     * }
     * ...
     * void setup() {
     *   ...
     *   I2C::onRequest(dataRequest);
     * }
     * \endcode
     * \note
     * Interrupts are disabled during this callback.
     * Any functions called within it should not rely on interrupts (i.e. no `Serial`, `delay`, `millis`, etc.).
     * To respond to the controller (master), use `reply` instead of `write`.
     * \see onReceive() reply() read()
     */
    static void onRequest(void (*function)());

    /** \brief
     * Sets up/disables the callback to be called when data is sent to the device's address (a write).
     * \param function The function to be called when data is received, or nullptr to disable.
     * \details
     * Example Callback and Usage:
     * \code{.cpp}
     * void dataReceive() {
     *     const uint8_t *buffer = I2C::getBuffer();
     * }
     * ...
     * void setup() {
     *   ...
     *   I2C::onReceive(dataReceive);
     * }
     * \endcode
     * \see onRequest() reply() read()
     */
    static void onReceive(void (*function)());

    /** \brief
     * Gets the hardware error which happened in a previous read or write.
     * \return A byte indicating the error. \n
     * Options:
     * - `I2C_ERROR_WRITE_ADDR_NACK`: Target (slave) did not acknowledge its address during a write; it is not connected or is not responding. \n
     * - `I2C_ERROR_WRITE_DATA_NACK`: Target (slave) did not acknowledge the data sent to it during a write; it disconnected or hung in the middle of receiving. \n
     * - `I2C_ERROR_READ_ADDR_NACK`: Target (slave) did not acknowledge its address during a read; it is not connected or is not responding. \n
     * - `I2C_ERROR_ARB_LOST`: This device lost arbitration while trying to become the controller (master); another device is already the controller (master). \n
     * - `I2C_ERROR_BUS`: An illegal start or stop condition was detected on the bus. \n
     * - `I2C_ERROR_NONE`: No error has occurred. \n
     */
    static uint8_t getError();

    /** \brief
     * Gets a pointer to the internal buffer used for I2C communication.
     * \return A pointer to the internal buffer.
     * \details
     * This function is intended to be used in the onReceive callback to get the data sent by the controller (master).
     * \see onReceive()
     */
    static const uint8_t *getBuffer();

    /** \brief
     * Gets the size of the data in the internal buffer.
     * \return The size of the data in the internal buffer.
     * \details
     * This function is intended to be used in the onReceive callback to get the size of the data sent by the controller (master).
     * \see onReceive()
     */
    static uint8_t getBufferSize();

    /** \brief
     * Checks if the I2C cable is flipped, calling a function if it is and waiting for it to be flipped back.
     * \param function The function to be called if the cable is flipped.
     * \details
     * This function works by seeing which line behaves more like a clock (equal high and low) over a sampling period.
     * It is by no means perfect, but it should suffice.
     * This is only needed on the FX-C, as the Arduboy Mini does not have a way to flip the cable.
     * This method must be used with I2C::handshake.
     * Example Usage:
     * \code{.cpp}
     * I2C::checkCableFlipped([] {
     *     arduboy.clear();
     *     arduboy.print(F("Please flip the cable\non this device."));
     *     arduboy.display();
     * });
     * uint8_t id = I2C::handshake();
     * \endcode
     * \note
     * In order to work with this function, custom handshaking functions must send data at a regular interval.
     * Sending 0b00000000 is recommended as it will increase the chance of detection.
     */
    static void checkCableFlipped(void (*function)());

    /** \brief
     * Checks if an emulator without I2C support is being used to run the code.
     * \return True if an emulator without I2C support has been detected and false if it has not
     */
    static bool checkEmulator();

    /** \brief
     * Gets the address from a provided id.
     * \param id An id between 0 and I2C_MAX_IDS - 1.
     * \return The address corresponding to that id.
     * \details
     * This function is provided to standardize addresses for each id. It is used by I2C::handshake.
     */
    static uint8_t idToAddress(uint8_t id);

    /** \brief
     * Handshakes with other devices and returns a unique id once complete.
     * \param numPlayers The number of players to wait for before completing the handshake. Must be between 1 and I2C_MAX_IDS. Defaults to 2.
     * \return A unique id for this device.
     * \details
     * This function may be called only once; further calls will not work.
     * This function will wait until every single player has joined.
     * \note
     * The onReceive() callback will be overridden by this function.
     */
    static uint8_t handshake(uint8_t numPlayers = 2);
};

#ifdef I2C_IMPLEMENTATION
/// \cond
/** \brief
 * Not officially part of the library.
 */
namespace i2c_detail {
#if I2C_USE_HANDSHAKE
volatile uint8_t handshakeState;

void handshakeOnRequest() {
    handshakeState++;
}
#endif // #if I2C_USE_HANDSHAKE

struct i2c_data_t {
    void (*onRequestFunction)() = nullptr;
    void (*onReceiveFunction)() = nullptr;

    volatile uint8_t *readBuffer;
    uint8_t twiBuffer[I2C_BUFFER_CAPACITY];
    volatile uint8_t bufferIdx;
    volatile uint8_t bufferSize;

    volatile uint8_t active;
    volatile uint8_t slaRW;
    volatile uint8_t error;

} data;

#if I2C_USE_MULTI_CONTROLLER
bool checkBusBusy() {
    uint8_t busyChecks = I2C_MULTI_CONTROLLER_BUSY_CHECKS;
    while (busyChecks) {
        if ((I2C_PIN & (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT))) ==
            (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT))) {
            busyChecks--;
        } else {
            i2c_detail::data.error = I2C_ERROR_ARB_LOST;
            i2c_detail::data.active = false;
            return true;
        }
        _delay_us(1000000.0 / I2C_FREQUENCY / 2.0);
    }
    return false;
}
#endif // #if I2C_USE_MULTI_CONTROLLER

#if I2C_USE_CHECK_CABLE_FLIPPED
bool checkCableFlippedCore(bool disconnectFlip = false) {
    uint8_t prev = I2C_PIN;
    uint8_t sdaEdges = 0;
    uint8_t sclEdges = 0;

    for (uint8_t i = 0; i < I2C_CHECK_CABLE_FLIPPED_CHECKS; i++) {
        uint8_t cur = I2C_PIN;
        uint8_t diff = cur ^ prev;

        if (diff & _BV(I2C_SDA_BIT)) sdaEdges++;
        if (diff & _BV(I2C_SCL_BIT)) sclEdges++;

        prev = cur;
        _delay_us(1000000.0 / I2C_FREQUENCY / 2.0);
    }

    if (sdaEdges + sclEdges <= 2) {
        return disconnectFlip;
    }
    return sdaEdges > sclEdges;
}

// optimizes for debounce in checkCableFlipped (only needs uint16_t)
uint16_t millisShort() {
    return (uint16_t)millis();
}
#endif // #if I2C_USE_CHECK_CABLE_FLIPPED

void startReadWrite(uint8_t address, bool readWrite, uint8_t bufferSize) {
    while (i2c_detail::data.active) {}
    i2c_detail::data.active = true;

    i2c_detail::data.error = I2C_ERROR_NONE;
    i2c_detail::data.slaRW = address << 1 | readWrite;
    i2c_detail::data.bufferIdx = 0;
    i2c_detail::data.bufferSize = bufferSize;
}
}
/// \endcond

void I2C::begin() {
    power_twi_enable();
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);

    // clear prescaler bits
    TWSR = 0;

    TWBR = (F_CPU / I2C_FREQUENCY - 16) / 2;

#if I2C_USE_SOFTWARE_PULLUPS
    // enable software pullups
    I2C_DDR &= ~(_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT));
    I2C_PORT |= _BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT);
#endif // #if I2C_USE_SOFTWARE_PULLUPS
}

void I2C::end() {
    TWCR = 0;
    power_twi_disable();
#if I2C_USE_SOFTWARE_PULLUPS
    // disable software pullups
    I2C_PORT &= ~(_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT));
#endif // #if I2C_USE_SOFTWARE_PULLUPS
}

void I2C::setAddress(uint8_t address, bool generalCall) {
    TWAR = address << 1 | generalCall;
}

void I2C::write(uint8_t address, const void *buffer, uint8_t size, bool wait) {
    i2c_detail::startReadWrite(address, TW_WRITE, size);

    memcpy(i2c_detail::data.twiBuffer, buffer, size);

#if I2C_USE_MULTI_CONTROLLER
    if (i2c_detail::checkBusBusy()) { return; }
#endif // #if I2C_USE_MULTI_CONTROLLER
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWSTA) | _BV(TWINT);
    if (wait) {
        while (i2c_detail::data.active) {}
    }
}

void I2C::read(uint8_t address, void *buffer, uint8_t size) {
    i2c_detail::startReadWrite(address, TW_READ, size - 1);

    i2c_detail::data.readBuffer = (uint8_t *)buffer;

#if I2C_USE_MULTI_CONTROLLER
    if (i2c_detail::checkBusBusy()) { return; }
#endif // #if I2C_USE_MULTI_CONTROLLER
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWSTA) | _BV(TWINT);
    while (i2c_detail::data.active) {}
}

void I2C::reply(const void *buffer, uint8_t size) {
    memcpy(i2c_detail::data.twiBuffer, buffer, size);
    i2c_detail::data.bufferSize = size;
}

void I2C::onRequest(void (*function)()) {
    i2c_detail::data.onRequestFunction = function;
}

void I2C::onReceive(void (*function)()) {
    i2c_detail::data.onReceiveFunction = function;
}

uint8_t I2C::getError() {
    return i2c_detail::data.error;
}

const uint8_t *I2C::getBuffer() {
    return i2c_detail::data.twiBuffer;
}

uint8_t I2C::getBufferSize() {
    return i2c_detail::data.bufferSize;
}

#if I2C_USE_CHECK_CABLE_FLIPPED
void I2C::checkCableFlipped(void (*function)()) {
    // wait to finish ongoing operations
    while (i2c_detail::data.active) { }
    TWCR = 0; // disable TWI

    if (i2c_detail::checkCableFlippedCore()) {
        // inform the user of the flipped cable
        function();
        // wait for cable to be flipped back
        // debounce cable changes
        uint16_t start = i2c_detail::millisShort();
        while (true) {
            if (i2c_detail::checkCableFlippedCore(true)) {
                start = i2c_detail::millisShort();
            } else if (i2c_detail::millisShort() - start >
                    I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_TIMEOUT) {
                break;
            }
        }
    }

    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) |  _BV(TWINT); // re-enable TWI
}
#endif // #if I2C_USE_CHECK_CABLE_FLIPPED

bool I2C::checkEmulator() {
    // TWWC is set when TWDR is written to without TWINT being set
    // not done in emulator
    TWDR = 0;
    return !(TWCR & _BV(TWWC));
}

uint8_t I2C::idToAddress(uint8_t id) {
    return 0x8 + id;
}

#if I2C_USE_HANDSHAKE
uint8_t I2C::handshake(uint8_t numPlayers) {
    I2C::onRequest(i2c_detail::handshakeOnRequest);
    for (int8_t i = numPlayers - 1; i >= 0; ) {
        uint8_t dummy;
        uint8_t address = I2C::idToAddress(i);

        I2C::read(address, dummy);

        switch (I2C::getError()) {
        case I2C_ERROR_READ_ADDR_NACK:
            I2C::setAddress(address);

            // handshakeState is the number of times the callback has been called.
            // when the callback has been called i times, the final Arduboy has joined.
            // cable flipped detection relies on clock detection,
            // so we send 0b00000000 to have SDA change as little as possible
            // while detecting it.
#if I2C_USE_CHECK_CABLE_FLIPPED
            dummy = 0b00000000;
            while (i2c_detail::handshakeState < i) {
                I2C::write(I2C_GENERAL_CALL_ADDR, dummy, true);
                // avoid hogging the bus
                _delay_us(1000000.0 / I2C_FREQUENCY);
            }
#else
            while (i2c_detail::handshakeState < i) { }
#endif // #if I2C_USE_CHECK_CABLE_FLIPPED

            return i;
        case I2C_ERROR_NONE:
            i--;
            break;
        case I2C_ERROR_ARB_LOST:
            // we lost arbitration
            // break without decrementing i to try again
            break;
        default:
            // unknown error happened
            // break without decrementing i to try again
            break;
        }
    }
    return I2C_HANDSHAKE_FULL;
}
#endif // #if I2C_USE_HANDSHAKE

ISR(TWI_vect, ISR_NAKED) {
    asm volatile (
R"(
; --------------------- defines ----------------------- ;
.equ TWPTR, 0xB9
.equ TWCR, 3
.equ TWSR, 0
.equ TWDR, 2

.equ TWIE, 0
.equ TWEN, 2
.equ TWWC, 3
.equ TWSTO, 4
.equ TWSTA, 5
.equ TWEA, 6
.equ TWINT, 7

.equ REPLY_ACK, (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA)
.equ REPLY_NACK, (1 << TWINT) | (1 << TWEN) | (1 << TWIE)
.equ STOP, (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTO) | (1 << TWEA)

; -------------------- registers ---------------------- ;
; r18     - TWSR (never used after function call)
; r19     - general use
; r20     - REPLY_ACK
; r21     - REPLY_NACK
; r26 (X) - general use
; r27 (X) - general use
; r28 (Y) - data pointer
; r29 (Y) - data pointer
; r30 (Z) - TW register pointer
; r31 (Z) - TW register pointer
; --------------------- prologue ---------------------- ;
push r18
in r18, __SREG__
push r18
; save and restore call-clobbered registers
; target (slave) could call function pointer
push r19
push r20
push r21
push r22
push r23
push r24
push r25
push r26
push r27
push r28
push r29
push r30
push r31
; save and restore tmp and zero registers (could be used in function calls)
push __tmp_reg__
push __zero_reg__
clr __zero_reg__
; ----------------------------------------------------- ;
; set up Y pointer (data)
ldi r28, lo8(%[data])
ldi r29, hi8(%[data])

; set up Z pointer (TW registers)
ldi r30, TWPTR
clr r31

; set up r20 and r21 (REPLY_ACK and REPLY_NACK)
ldi r20, REPLY_ACK
ldi r21, REPLY_NACK

; switch (TWSR)
ldd r18, Z + TWSR ; no mask needed because prescaler bits are cleared

cpi r18, 0x08
breq TW_START

; MT_MR
cpi r18, 0x18
breq TW_MT_SLA_ACK
cpi r18, 0x28
breq TW_MT_DATA_ACK
cpi r18, 0x38
breq TW_MT_ARB_LOST ; same as TW_MR_ARB_LOST
cpi r18, 0x40
breq TW_MR_SLA_ACK
cpi r18, 0x50
breq TW_MR_DATA_ACK
cpi r18, 0x58
breq TW_MR_DATA_NACK

; 64 instruction limit on branches
rjmp SR_ST

TW_START:
    ; TWDR = i2c_detail::data.slaRW;
    ldd r26, Y + %[slaRW]
    std Z + TWDR, r26
    ; TWCR = REPLY_NACK;
    std Z + TWCR, r21
    ; return;
    rjmp pop_reti

TW_MT_SLA_ACK:
TW_MT_DATA_ACK:
    ; if (i2c_detail::data.bufferIdx >= i2c_detail::data.bufferSize) { stop(); return; }
    ldd r26, Y + %[bufferIdx]
    ldd r27, Y + %[bufferSize]
    cp r26, r27

    brlo 1f ; 64 instruction limit on branches
    rjmp stop_reti
    1:

    ; TWDR = i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++];
    call get_buffer_addr
    ld r26, X
    std Z + TWDR, r26

    ; TWCR = REPLY_NACK;
    std Z + TWCR, r21
    ; return;
    rjmp pop_reti

TW_MT_ARB_LOST:
    ; TWCR = REPLY_ACK;
    std Z + TWCR, r20
    ; i2c_detail::data.error = TW_MT_ARB_LOST;
    std Y + %[error], r18
    ; active = false;
    ; return;
    rjmp active_false_reti
; ----------------------------------------------------- ;

TW_MR_DATA_NACK:
TW_MR_DATA_ACK:
    ; i2c_detail::data.readBuffer[i2c_detail::data.bufferIdx++] = TWDR;
    ldd r19, Y + %[bufferIdx]
    ldd r26, Y + %[readBuffer]
    ldd r27, Y + %[readBuffer] + 1

    add r26, r19
    adc r27, __zero_reg__

    inc r19
    std Y + %[bufferIdx], r19

    ldd r19, Z + TWDR
    st X, r19

    ; if (TWSR == TW_MR_DATA_NACK) { stop(); return; }
    ; r18 holds TWSR
    cpi r18, 0x58
    brne 1f ; 64 instruction limit on branches
    rjmp stop_reti
    1:
; ------------------ fallthrough ---------------------- ;
TW_MR_SLA_ACK:
    ; if (i2c_detail::data.bufferIdx < i2c_detail::data.bufferSize) {
    ;    TWCR = REPLY_ACK;
    ; } else {
    ;    TWCR = REPLY_NACK;
    ; }
    ; return;

    ; r20 and r21 may be clobbered, do not use

    ldd r26, Y + %[bufferIdx]
    ldd r27, Y + %[bufferSize]
    cp r26, r27
    ldi r26, REPLY_ACK
    brlo 1f
    ldi r26, REPLY_NACK
    1:
    std Z + TWCR, r26
    rjmp pop_reti
; ----------------------------------------------------- ;
SR_ST:
cpi r18, 0x60
breq TW_SR_SLA_ACK
cpi r18, 0x68
breq TW_SR_ARB_LOST_SLA_ACK
cpi r18, 0x70
breq TW_SR_GCALL_ACK
cpi r18, 0x78
breq TW_SR_ARB_LOST_GCALL_ACK
cpi r18, 0x80
breq TW_SR_DATA_ACK
cpi r18, 0x90
breq TW_SR_GCALL_DATA_ACK
cpi r18, 0xA0
breq TW_SR_STOP
cpi r18, 0xA8
breq TW_ST_SLA_ACK
cpi r18, 0xB0
breq TW_ST_ARB_LOST_SLA_ACK
cpi r18, 0xB8
breq TW_ST_DATA_ACK
cpi r18, 0xC0
breq TW_ST_DATA_NACK
cpi r18, 0xC8
breq TW_ST_LAST_DATA

rjmp default

TW_SR_SLA_ACK:
TW_SR_ARB_LOST_SLA_ACK:
TW_SR_GCALL_ACK:
TW_SR_ARB_LOST_GCALL_ACK:
    ; i2c_detail::data.active = TWSR; (true)
    std Y + %[active], r18 ; r18 holds TWSR
    ; i2c_detail::data.bufferIdx = 0;
    std Y + %[bufferIdx], __zero_reg__
    ; TWCR = REPLY_ACK;
    std Z + TWCR, r20
    ; return;
    rjmp pop_reti

TW_SR_DATA_ACK:
TW_SR_GCALL_DATA_ACK:
    ; i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++] = TWDR;
    call get_buffer_addr
    ldd r19, Z + TWDR
    st X, r19

    ; TWCR = REPLY_ACK;
    std Z + TWCR, r20
    ; return;
    rjmp pop_reti
TW_SR_STOP:
    ; TWCR = REPLY_ACK;
    std Z + TWCR, r20

    ; if (i2c_detail::data.onReceiveFunction) {
    ;     i2c_detail::data.onReceiveFunction();
    ; }

    ldd r30, Y + %[onReceiveFunction]
    ldd r31, Y + %[onReceiveFunction] + 1

    cp r30, __zero_reg__
    cpc r31, __zero_reg__
    breq active_false_reti

    icall
    ; TW register pointer may be clobbered, do not use
    ; r20 and r21 may be clobbered, do not use

    ; i2c_detail::data.active = false;
    ; return;
    rjmp active_false_reti;
; ----------------------------------------------------- ;
TW_ST_ARB_LOST_SLA_ACK:
TW_ST_SLA_ACK:
    ; i2c_detail::data.active = TWSR; (true)
    std Y + %[active], r18
    ; i2c_detail::data.bufferIdx = 0;
    std Y + %[bufferIdx], __zero_reg__
    ; default to sending 1 junk byte if the user does not fill buffer
    ; i2c_detail::data.bufferSize = 1;
    ldi r26, 1
    std Y + %[bufferSize], r26

    ; if (i2c_detail::data.onRequestFunction) {
    ;     i2c_detail::data.onRequestFunction();
    ; }
    ; i2c_detail::data.onRequestFunction();
    ldd r30, Y + %[onRequestFunction]
    ldd r31, Y + %[onRequestFunction] + 1

    cp r30, __zero_reg__
    cpc r31, __zero_reg__
    breq 1f
    icall

    ; restore Z pointer
    ldi r30, TWPTR
    clr r31
    ; r20 and r21 may be clobbered, do not use
    1:

    ; ------------------ fallthrough ---------------------- ;
TW_ST_DATA_ACK:
    ; TWDR = i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++];
    call get_buffer_addr
    ld r26, X
    std Z + TWDR, r26

    ; if (i2c_detail::data.bufferIdx < i2c_detail::data.bufferSize) {
    ;    TWCR = REPLY_ACK;
    ; } else {
    ;    TWCR = REPLY_NACK;
    ; }
    ; return;
    ; (reuse code in MR)
    rjmp TW_MR_SLA_ACK
TW_ST_DATA_NACK:
TW_ST_LAST_DATA:
    ; TWCR = REPLY_ACK;
    ldi r26, REPLY_ACK
    std Z + TWCR, r26
    ; i2c_detail::data.active = false;
    ; return;
    rjmp active_false_reti
; ----------------------------------------------------- ;
default:
    ; i2c_detail::data.error = TWSR;
    std Y + %[error], r18

    stop_reti:

    ; TWCR = STOP;
    ldi r26, STOP
    std Z + TWCR, r26

    ; while (TWCR & _BV(TWSTO)) {}
    1:
    ldd r26, Z + TWCR
    sbrc r26, TWSTO ; skip if bit in register clear
    rjmp 1b

    active_false_reti:
    ; Z pointer may be clobbered, do not use
    ; r20 and r21 may be clobbered, do not use

    ; i2c_detail::data.active = false;
    std Y + %[active], __zero_reg__

; --------------------- epilogue ---------------------- ;
    pop_reti:
    pop __zero_reg__
    pop __tmp_reg__
    pop r31
    pop r30
    pop r29
    pop r28
    pop r27
    pop r26
    pop r25
    pop r24
    pop r23
    pop r22
    pop r21
    pop r20
    pop r19
    pop r18
    out __SREG__, r18
    pop r18
    reti
; -------------------- subroutines --------------------- ;
; output: X = &twiBuffer[bufferIdx++]
get_buffer_addr:
    ; TWDR = i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++];
    ldd r26, Y + %[bufferIdx]
    inc r26
    std Y + %[bufferIdx], r26

    ; use SUBI and SBCI as (non-existent) ADDI and (non-existent) ADCI
    ; bufferIdx is already incremented so decrement to compensate

    clr r27
    subi r26, lo8(-(%[twiBuffer] - 1))
    sbci r27, hi8(-(%[twiBuffer] - 1))
    ret
)"
        : // Output Operands
        [data]             "=m" (i2c_detail::data),
        [twiBuffer]        "=m" (i2c_detail::data.twiBuffer)
        : // Input Operands
        [error]             "i" (offsetof(i2c_detail::i2c_data_t, error)),
        [active]            "i" (offsetof(i2c_detail::i2c_data_t, active)),
        [bufferIdx]         "i" (offsetof(i2c_detail::i2c_data_t, bufferIdx)),
        [readBuffer]          "i" (offsetof(i2c_detail::i2c_data_t, readBuffer)),
        [onRequestFunction] "i" (offsetof(i2c_detail::i2c_data_t, onRequestFunction)),
        [onReceiveFunction] "i" (offsetof(i2c_detail::i2c_data_t, onReceiveFunction)),
        [bufferSize]        "i" (offsetof(i2c_detail::i2c_data_t, bufferSize)),
        [slaRW]             "i" (offsetof(i2c_detail::i2c_data_t, slaRW))
    );
}

// C++ ISR version for reference
#if 0
ISR(TWI_vect) {
    switch (TWSR) { // prescaler bits are cleared, no mask needed
    case TW_START:
        TWDR = i2c_detail::data.slaRW;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
        break;
    // MT
    case TW_MT_SLA_ACK:
    case TW_MT_DATA_ACK:
        if (i2c_detail::data.bufferIdx < i2c_detail::data.bufferSize) {
            TWDR = i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++];
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
        } else {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO) | _BV(TWEA);
            while (TWCR & _BV(TWSTO)) {  }
            i2c_detail::data.active = false;
        }
        break;
    case TW_MT_ARB_LOST: // same as TW_MR_ARB_LOST
        i2c_detail::data.active = false;
        i2c_detail::data.error = TW_MT_ARB_LOST;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;
    // MR
    case TW_MR_DATA_ACK:
        i2c_detail::data.readBuffer[i2c_detail::data.bufferIdx++] = TWDR;
        __attribute__((fallthrough));
    case TW_MR_SLA_ACK:
        if (i2c_detail::data.bufferIdx < i2c_detail::data.bufferSize) {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        } else {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
        }
        break;
    case TW_MR_DATA_NACK:
        i2c_detail::data.readBuffer[i2c_detail::data.bufferIdx++] = TWDR;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO) | _BV(TWEA);
        while (TWCR & _BV(TWSTO)) {  }
        i2c_detail::data.active = false;
        break;
    // ST
    case TW_ST_SLA_ACK:
    case TW_ST_ARB_LOST_SLA_ACK:
        i2c_detail::data.active = true;
        i2c_detail::data.bufferIdx = 0;
        i2c_detail::data.bufferSize = 1; // default to sending 1 junk byte if the user does not fill buffer
        if (i2c_detail::data.onRequestFunction) {
            i2c_detail::data.onRequestFunction();
        }
        __attribute__((fallthrough));
    case TW_ST_DATA_ACK:
        TWDR = i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++];
        if (i2c_detail::data.bufferIdx < i2c_detail::data.bufferSize) {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        } else {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
        }
        break;
    case TW_ST_DATA_NACK:
    case TW_ST_LAST_DATA: // last interrupt cleared TWEA
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        i2c_detail::data.active = false;
        break;
    // SR
    case TW_SR_SLA_ACK:
    case TW_SR_GCALL_ACK:
    case TW_SR_ARB_LOST_SLA_ACK:
    case TW_SR_ARB_LOST_GCALL_ACK:
        i2c_detail::data.bufferIdx = 0;
        i2c_detail::data.active = true;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;
    case TW_SR_GCALL_DATA_ACK:
    case TW_SR_DATA_ACK:
        i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++] = TWDR;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;
    case TW_SR_STOP:
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        if (i2c_detail::data.onReceiveFunction) {
            i2c_detail::data.onReceiveFunction();
        }
        i2c_detail::data.active = false;
        break;
    default:
        i2c_detail::data.error = TWSR;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO) | _BV(TWEA);
        while (TWCR & _BV(TWSTO)) {  }
        i2c_detail::data.active = false;
        break;
    }
}
#endif // #if 0

#endif // #ifdef I2C_IMPLEMENTATION
