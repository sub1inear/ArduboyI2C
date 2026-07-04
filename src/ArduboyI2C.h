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
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/twi.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef I2C_FREQUENCY
/** \brief
 * The initial I2C frequency in Hz.
 * \details
 * Defaults to 100000 Hz, with a maximum of 300000 Hz.
 * The Arduboy FX-C only has software pullups, so the I2C frequency must be < 300000 Hz to ensure reliability.
 */
#define I2C_FREQUENCY 100000
#elif I2C_FREQUENCY > 300000
#error I2C_FREQUENCY is too high.
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
#error I2C_BUFFER_CAPACITY is too big.
#endif

#ifndef I2C_CHECK_CABLE_FLIPPED_CHECKS
/** \brief
 * The total number of checks to perform when checking for a flipped cable.
 * \details
 * Defaults to 128, with a maximum of 65535. Increase for a more accurate detection at the cost of a longer detection time.
 */
#define I2C_CHECK_CABLE_FLIPPED_CHECKS 128
#elif I2C_CHECK_CABLE_FLIPPED_CHECKS > 65535
#error I2C_CHECK_CABLE_FLIPPED_CHECKS is too big.
#endif

#ifndef I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_CHECKS
/** \brief
 * The number of checks to perform when debouncing the cable flip detection.
 * \details
 * Defaults to 128, with a maximum of 65535.
 * Increase for more accurate debouncing at the cost of a longer detection time.
 */
#define I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_CHECKS 128
#elif I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_CHECKS > 65535
#error I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_CHECKS is too big.
#endif

#ifndef I2C_HANDSHAKE_BUSY_CHECKS
/** \brief
 * The total number of busy checks to perform when waiting for a handshake.
 * \details
 * Defaults to 128, with a maximum of 65535. Increase for a more accurate detection at the cost of a longer detection time,
 * especially when a custom loop function is provided to the handshake function.
 */
#define I2C_HANDSHAKE_BUSY_CHECKS 128
#elif I2C_HANDSHAKE_BUSY_CHECKS > 65535
#error I2C_HANDSHAKE_BUSY_CHECKS is too big.
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
 * The target address set by the handshake function.
 * \details
 * This is the address that the controller (master) should use to communicate with the target (slave) when using the handshake function.
 * This is optional; the address can be changed to whatever you wish but is kept for convenience.
 */
#define I2C_TARGET_ADDRESS 0x08

/** \brief
 * The null address used to disable a target from responding.
 * \details
 * This is useful in lockstep multiplayer to soft-disable a target (slave) from responding to the controller (master) without having to call `I2C::end()`.
 * This is only a convention and is optional; the address can be changed to whatever you wish but is kept for convenience.
 */
#define I2C_NULL_ADDRESS 0x09

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
 * \details
 * Error code returned by I2C::getError().
 * \note
 * This code has no distinction between reads and writes (hardware constraint).
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
 * \details
 */
#define I2C_ERROR_NONE 0xFF

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
     * It may be reinitialized by calling begin() again.
     * \see begin()
     */
    static void end();

    /** \brief
     * Sets the 7-bit address of this device.
     * \param address The 7-bit address (0-127) to set.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \see getAddress()
     */
    static void setAddress(uint8_t address);

    /** \brief
     * Gets the 7-bit address of this device.
     * \return The 7-bit address of this device.
     * \see setAddress()
     */
    static uint8_t getAddress();

    /** \brief
     * Attempts to become the bus controller (master) and sends data over I2C to the specified address.
     * \param address The 7-bit address to send the data to.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \param buffer A pointer to the data to send.
     * \param size The amount of data in bytes to send. This cannot be zero.
     * \param wait Whether or not to wait for the write to complete. If this is false, it will proceed with interrupts.
     * \note
     * Internally, this function uses a buffer to enable asynchronous writes.
     * The buffer size is controlled by the macro I2C_BUFFER_CAPACITY and defaults to 32.
     * If the program needs to send more than 32 bytes at a time, I2C_BUFFER_CAPACITY
     * must be defined before including ArduboyI2C.h to be greater.
     * \note
     * To poll whether asynchronous writes have completed, see if getActive() returns false.
     * \note
     * This function will not work inside the onRequest() callback. Send data with reply() instead.
     * \see reply() read() getActive()
     */
    static void write(uint8_t address, const void *buffer, uint8_t size, bool wait);

    /** \overload
     * \tparam T The type of the object to send. To prevent bugs, T cannot be a pointer.
     * \param address The 7-bit address to send the data to.
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

    /** \overload
     * \tparam T The type of the array to send.
     * \tparam N The number of elements in the array.
     * \param address The 7-bit address to send the data to.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \param buffer A reference to the array to send.
     * \param wait Whether or not to wait for the write to complete. If this is false, it will proceed with interrupts.
     * \details
     * This function will automatically deduce the size of the array.
     * Arrays with sizes greater than or equal to 255 should not be used with this function.
     */
    template<typename T, size_t N>
    static void write(uint8_t address, const T (&buffer)[N], bool wait) {
        // N must be size_t otherwise const T & variant captures overload with N > 255
        static_assert(sizeof(T) * N <= I2C_BUFFER_CAPACITY, "Size of T * N must be less than or equal to I2C_BUFFER_CAPACITY.");
        I2C::write(address, (const void *)buffer, sizeof(T) * N, wait);
    }

    /** \brief
     * Attempts to become the bus controller (master) and reads data over I2C from the specified address.
     * \param address The 7-bit address to receive the data from.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \param buffer A pointer to the buffer in which to store the data.
     * \param size The maximum amount of bytes to receive. This cannot be 0 or 255.
     * \note
     * Unlike the `write` function, this function is bufferless and is not limited to `I2C_BUFFER_CAPACITY` bytes.
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

    /** \overload
     * \tparam T The type of the array to read.
     * \tparam N The number of elements in the array.
     * \param address The 7-bit address to receive the data from.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \param buffer A reference to the array in which to store the data.
     * \details
     * This function will automatically deduce the size of the array.
     * Arrays with sizes greater than or equal to 255 should not be used with this function.
     */
    template<typename T, size_t N>
    static void read(uint8_t address, T (&buffer)[N]) {
        // N must be size_t otherwise const T & variant captures overload with N > 255
        static_assert(sizeof(T) * N < 255, "Size of T * N must be less than 255.");
        I2C::read(address, (void *)buffer, sizeof(T) * N);
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
     * must be defined before including ArduboyI2C.h to be greater.
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

    /** \overload
     * \tparam T The type of the array to reply with.
     * \tparam N The number of elements in the array.
     * \param buffer A reference to the array to reply with.
     * \details
     * This function will automatically deduce the size of the array.
     * Arrays with sizes greater than or equal to 255 should not be used with this function.
     */
    template<typename T, size_t N>
    static void reply(const T (&buffer)[N]) {
        // N must be size_t otherwise const T & variant captures overload with N > 255
        static_assert(sizeof(T) * N <= I2C_BUFFER_CAPACITY, "Size of T * N must be less than or equal to I2C_BUFFER_CAPACITY.");
        I2C::reply((const void *)buffer, sizeof(T) * N);
    }

    /** \brief
     * Sets up/disables the callback to be called when data is requested from the device's address (a read).
     * \param function The function to be called when data is requested, or `nullptr` to disable.
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
     * To respond to the controller (master), use reply() instead of write().
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
     * \note
     * Interrupts are disabled during this callback.
     * Any functions called within it should not rely on interrupts (i.e. no `Serial`, `delay`, `millis`, etc.).
     * \see onRequest() read()
     */
    static void onReceive(void (*function)());

    /** \brief
     * Gets the hardware error which happened in a previous read or write.
     * \return A byte indicating the error. \n
     * Options:
     * - `I2C_ERROR_WRITE_ADDR_NACK`: Target (slave) did not acknowledge its address during a write; it is not connected or is not responding. \n
     * - `I2C_ERROR_WRITE_DATA_NACK`: Target (slave) did not acknowledge the data sent to it during a write; it disconnected or hung in the middle of receiving. \n
     * - `I2C_ERROR_READ_ADDR_NACK`: Target (slave) did not acknowledge its address during a read; it is not connected or is not responding. \n
     * - `I2C_ERROR_BUS`: An illegal start or stop condition was detected on the bus. \n
     * - `I2C_ERROR_NONE`: No error has occurred. \n
     */
    static uint8_t getError();

    /** \brief
     * Gets a pointer to the internal buffer used for I2C communication.
     * \return A pointer to the internal buffer.
     * \details
     * This function is intended to be used in the onReceive callback to get the data sent by the controller (master).
     * \note
     * The returned pointer may be safely cast to remove the `const` if needed as scratch space.
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
     * Gets whether or not the controller (master) is currently transmitting or receiving data.
     * \return `true` if the controller (master) is currently transmitting or receiving data, `false` otherwise.
     * \details
     * This function is intended to be used to check if an asynchronous write has completed.
     * \see write()
     */
    static bool getActive();

    /** \brief
     * Checks if the I2C cable is flipped, calling a function if it is and waiting for it to be flipped back.
     * \param startFunction The function to be called if the cable is flipped. Pass `nullptr` to disable.
     * \param loopFunction The function to be called while waiting for the cable to be flipped back. Pass `nullptr` to disable.
     * \details
     * This is only needed on the FX-C, as the Arduboy Mini does not have a way to flip the cable.
     * This method must be used with I2C::handshake or an equivalent handshaking method that sends `0b00000000` at a regular interval.
     * Example Usage:
     * \code{.cpp}
     * I2C::checkCableFlipped([] {
     *     arduboy.clear();
     *     arduboy.print(F("Please flip the cable\non this device."));
     *     arduboy.display();
     * });
     * bool isController = I2C::handshake();
     * \endcode
     * \note
     * Custom functions should not call I2C functions. They may rely on interrupts.
     * Setting a custom loop function is advanced and requires careful tuning of the I2C_*_CHECKS macros to ensure everything works reliably.
     * Loop functions should be as fast as possible; it is not recommended to use arduboy.nextFrame() within a loop function.
     */
    static void checkCableFlipped(void (*startFunction)() = nullptr, void (*loopFunction)() = nullptr);

    /** \brief
     * Waits for another device and returns whether this device is the controller (master) or target (slave).
     * \param startFunction The function to be called while waiting for another device. Pass `nullptr` to disable.
     * \param loopFunction The function to be called while waiting for another device. Pass `nullptr` to disable.
     * \return `true` if this device is the controller (master), `false` if it is the target (slave).
     * \note
     * Custom functions should not call I2C functions. They may rely on interrupts.
     * Setting a custom loop function is advanced and requires careful tuning of the I2C_*_CHECKS macros to ensure everything works reliably.
     * Loop functions should be as fast as possible; it is not recommended to use `arduboy.nextFrame()` within a loop function.
     */
    static bool handshake(void (*startFunction)() = nullptr, void (*loopFunction)() = nullptr);
};

#ifdef I2C_IMPLEMENTATION
/// \cond
/** \brief
 * Not officially part of the library.
 */
namespace i2c_detail {
struct i2c_data_t {
    /** \brief
     * The function to be called when data is requested from the device's address (a read).
     */
    void (*onRequestFunction)() = nullptr;

    /** \brief
     * The function to be called when data is sent to the device's address (a write).
     */
    void (*onReceiveFunction)() = nullptr;

    /** \brief
     * A pointer to the buffer for reads.
     * This avoids copy into twiBuffer then to another buffer out, and is safe because I2C::read blocks.
     */
    volatile uint8_t *readBuffer;

    /** \brief
     * A buffer for storing data to be sent.
     */
    uint8_t twiBuffer[I2C_BUFFER_CAPACITY];

    /** \brief
     * The index into the readBuffer or twiBuffer.
     */
    volatile uint8_t bufferIdx;

    /** \brief
     * The size of data in readBuffer or twiBuffer.
     */
    volatile uint8_t bufferSize;

    /** \brief
     * Whether or not the controller (master) is currently transmitting or receiving data.
     * Not active for target (slave) operations.
     */
    volatile bool active;

    /** \brief
     * The address and read/write bit for a transmission (7-bit address << 1 | read/write bit).
     */
    volatile uint8_t slaRW;

    /** \brief
     * The error code for the last transmission.
     */
    volatile uint8_t error;
} data;

/** \brief
 * Counts the number of edges on the SDA and SCL lines to determine if the cable is flipped.
 * Relies on I2C::handshake() sending 0b00000000 so that the SCL line is toggled and the SDA line is held low.
 */
bool checkCableFlippedCore(bool disconnectFlip) {
    uint8_t prev = I2C_PIN;
    uint8_t sdaEdges = 0;
    uint8_t sclEdges = 0;

    // uint16_t will optimize to a uint8_t if I2C_CHECK_CABLE_FLIPPED_CHECKS < 256
    for (uint16_t i = 0; i < I2C_CHECK_CABLE_FLIPPED_CHECKS; i++) {
        uint8_t cur = I2C_PIN;
        // calculates the change between the current and previous pin states
        uint8_t diff = cur ^ prev;

        // if the SDA line changed, increment the SDA edge counter
        if (diff & _BV(I2C_SDA_BIT)) { sdaEdges++; }
        // if the SCL line changed, increment the SCL edge counter
        if (diff & _BV(I2C_SCL_BIT)) { sclEdges++; }

        prev = cur;
        // half-period delay otherwise too fast
        _delay_us(1000000.0 / I2C_FREQUENCY / 2.0);
    }

    // if the total number of edges is very low (<= 2 to account for noise), the cable is disconnected or flipped
    if (sdaEdges + sclEdges <= 2) {
        // returns flipped (true) if disconnect is a flip, otherwise returns not flipped (false)
        return disconnectFlip;
    }
    // otherwise, if the number of SDA edges is greater than the number of SCL edges, the cable is flipped
    return sdaEdges > sclEdges;
}

void startReadWrite(uint8_t address, bool readWrite, uint8_t bufferSize) {
    i2c_detail::data.active = true;
    i2c_detail::data.error = I2C_ERROR_NONE;
    i2c_detail::data.slaRW = address << 1 | readWrite;
    i2c_detail::data.bufferIdx = 0;
    i2c_detail::data.bufferSize = bufferSize;
}

void waitActive() {
    while (i2c_detail::data.active) { }
}

void sendStart() {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWSTA) | _BV(TWINT);
}

}
/// \endcond

void I2C::begin() {
    // power up TWI
    // power_twi_disable() is called in arduboy.boot() (inside arduboy.begin())
    power_twi_enable();

    // enable TWI, interrupts, ACKing
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
    // set our clock frequency
    TWBR = (F_CPU / I2C_FREQUENCY - 16) / 2;

    // enable software pullups (needed for Arduboy FX-C)
    I2C_DDR &= ~(_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT));
    I2C_PORT |= _BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT);
}

void I2C::end() {
    // disable TWI
    TWCR = 0;

    // power down TWI
    power_twi_disable();

    // disable software pullups (needed for Arduboy FX-C)
    // no need to change DDR
    I2C_PORT &= ~(_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT));
}

void I2C::setAddress(uint8_t address) {
    // TWAR format: 7-bit address in bits 7-1, TWGCE in bit 0
    TWAR = address << 1;
}

uint8_t I2C::getAddress() {
    // TWAR format: 7-bit address in bits 7-1, TWGCE in bit 0
    return TWAR >> 1;
}

void I2C::write(uint8_t address, const void *buffer, uint8_t size, bool wait) {
    i2c_detail::waitActive();

    i2c_detail::startReadWrite(address, TW_WRITE, size);
    memcpy(i2c_detail::data.twiBuffer, buffer, size);

    i2c_detail::sendStart();

    if (wait) {
        i2c_detail::waitActive();
    }
}

void I2C::read(uint8_t address, void *buffer, uint8_t size) {
    i2c_detail::waitActive();

    // ISR: TWCR = i2c_detail::bufferIdx < i2c_detail::bufferSize ? REPLY_ACK : REPLY_NACK;
    // so we must set the buffer size to size - 1 so that the last byte is NACKed (I2C standard)
    i2c_detail::startReadWrite(address, TW_READ, size - 1);
    i2c_detail::data.readBuffer = (uint8_t *)buffer;

    i2c_detail::sendStart();

    i2c_detail::waitActive();
}

void I2C::reply(const void *buffer, uint8_t size) {
    // save memory by not accumulating inside buffer (I2C::reply() may not be repeated)
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
    // i2c_detail::data.bufferSize is undefined during a target (slave) receive
    // i2c_detail::data.bufferIdx is incremented and therefore represents the current size in the buffer
    return i2c_detail::data.bufferIdx;
}

bool I2C::getActive() {
    return i2c_detail::data.active;
}

void I2C::checkCableFlipped(void (*startFunction)(), void (*loopFunction)()) {
    // disable TWI
    TWCR = 0;

    // check if cable is flipped
    // pass false to indicate a disconnect is not a flip
    // so it will return false and we will continue
    if (i2c_detail::checkCableFlippedCore(false)) {
        if (startFunction) {
            startFunction();
        }
        // wait for the cable to be flipped back
        // debounce the cable flip detection by ensuring 64 counts of success
        // putting the cable back it can generate noise
        // uint16_t will optimize to a uint8_t if I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_CHECKS < 256
        for (uint16_t i = 0; i < I2C_CHECK_CABLE_FLIPPED_DEBOUNCE_CHECKS; i++) {
            // pass true to indicate a disconnect is a flip
            // so we will wait while the cable is disconnected
            if (i2c_detail::checkCableFlippedCore(true)) {
                i = 0;
            }
            if (loopFunction) {
                loopFunction();
            }
        }
    }

    // re-enable TWI
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

bool I2C::handshake(void (*startFunction)(), void (*loopFunction)()) {
    // check if the bus is free (SDA and SCL are high for 128 half-periods)
    // if it is not, another controller (master) has gotten here before us
    // uint16_t will optimize to a uint8_t if I2C_HANDSHAKE_BUSY_CHECKS < 256
    for (uint16_t i = 0; i < I2C_HANDSHAKE_BUSY_CHECKS; i++) {
        if ((I2C_PIN & (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT))) !=
            (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT))) {
            // set our address to the target address
            I2C::setAddress(I2C_TARGET_ADDRESS);
            // wait for twice the time it takes to send 18 bits
            // (7 address bits + 1 RW + 1 ACK + 8 data bits + 1 ACK) to ensure we ACK the controller
            // might have come across a controller that just started sending data,
            // so we will NACK the first transmission and the ACK the second transmission
            // so we need to wait for two transmissions
            _delay_us((1000000.0 / I2C_FREQUENCY * 18.0 + 25.0) * 2.0);
            // return `false` to indicate that this device is the target (slave)
            return false;
        }
        // half-period delay otherwise too fast
        _delay_us(1000000.0 / I2C_FREQUENCY / 2.0);
    }
    uint8_t zeros = 0b00000000;
    if (startFunction) {
        startFunction();
    }
    do {
        // send all zeros to the target address to help with I2C::checkCableFlipped()
        I2C::write(I2C_TARGET_ADDRESS, zeros, true);
        if (loopFunction) {
            loopFunction();
        }
        // repeat until the write is successful (no NACKs)
    } while (I2C::getError() != I2C_ERROR_NONE);
    // return `true` to indicate that this device is the controller (master)
    return true;
}

#if 1
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
    ; if (i2c_detail::data.bufferIdx >= i2c_detail::data.bufferSize) {
    ;    stop();
    ;    return;
    ; }
    ldd r26, Y + %[bufferIdx]
    ldd r27, Y + %[bufferSize]
    cp r26, r27

    brlo 1f ; 64 instruction limit on branches
    rjmp stop_reti
    1:

    ; TWDR = i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++];
    rcall get_buffer_addr
    ld r26, X
    std Z + TWDR, r26

    ; TWCR = REPLY_NACK;
    std Z + TWCR, r21
    ; return;
    rjmp pop_reti
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

    ; if (TWSR == TW_MR_DATA_NACK) {
    ;     stop();
    ;     return;
    ; }
    ; r18 holds TWSR
    cpi r18, 0x58
    breq stop_reti
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
cpi r18, 0x80
breq TW_SR_DATA_ACK
cpi r18, 0xA0
breq TW_SR_STOP
cpi r18, 0xA8
breq TW_ST_SLA_ACK
cpi r18, 0xB8
breq TW_ST_DATA_ACK
cpi r18, 0xC0
breq TW_ST_DATA_NACK
cpi r18, 0xC8
breq TW_ST_LAST_DATA

rjmp default

TW_SR_SLA_ACK:
    ; i2c_detail::data.bufferIdx = 0;
    std Y + %[bufferIdx], __zero_reg__
    ; TWCR = REPLY_ACK;
    std Z + TWCR, r20
    ; return;
    rjmp pop_reti

TW_SR_DATA_ACK:
    ; i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++] = TWDR;
    rcall get_buffer_addr
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
    ; r20, r21, and r22 may be clobbered, do not use

    ; i2c_detail::data.active = false;
    ; return;
    rjmp active_false_reti;
; ----------------------------------------------------- ;
TW_ST_SLA_ACK:
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
    ; r20, r21, and r22 may be clobbered, do not use
    1:

    ; ------------------ fallthrough ---------------------- ;
TW_ST_DATA_ACK:
    ; TWDR = i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++];
    rcall get_buffer_addr
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
    ; r20, r21, and r22 may be clobbered, do not use

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
#if 0
R"(
debug_green:
    cbi 0x05, 7
    ret
debug_red:
    cbi 0x05, 6
    ret
debug_blue:
    cbi 0x05, 5
    ret

)"
#endif // #if 0
        : // Output Operands
        [data]             "=m" (i2c_detail::data),
        [twiBuffer]        "=m" (i2c_detail::data.twiBuffer)
        : // Input Operands
        [error]             "i" (offsetof(i2c_detail::i2c_data_t, error)),
        [active]            "i" (offsetof(i2c_detail::i2c_data_t, active)),
        [bufferIdx]         "i" (offsetof(i2c_detail::i2c_data_t, bufferIdx)),
        [readBuffer]        "i" (offsetof(i2c_detail::i2c_data_t, readBuffer)),
        [onRequestFunction] "i" (offsetof(i2c_detail::i2c_data_t, onRequestFunction)),
        [onReceiveFunction] "i" (offsetof(i2c_detail::i2c_data_t, onReceiveFunction)),
        [bufferSize]        "i" (offsetof(i2c_detail::i2c_data_t, bufferSize)),
        [slaRW]             "i" (offsetof(i2c_detail::i2c_data_t, slaRW))
    );
}
#else
// C++ ISR version for reference
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
        break;
    // SR
    case TW_SR_SLA_ACK:
        i2c_detail::data.bufferIdx = 0;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;
    case TW_SR_DATA_ACK:
        i2c_detail::data.twiBuffer[i2c_detail::data.bufferIdx++] = TWDR;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;
    case TW_SR_STOP:
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        if (i2c_detail::data.onReceiveFunction) {
            i2c_detail::data.onReceiveFunction();
        }
        break;
    default:
        i2c_detail::data.error = TWSR;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO) | _BV(TWEA);
        while (TWCR & _BV(TWSTO)) {  }
        i2c_detail::data.active = false;
        break;
    }
}
#endif // #if 1

#endif // #ifdef I2C_IMPLEMENTATION
