/*
MIT License

Copyright (c) 2024 sub1inear

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
#include <stdint.h>

#ifndef I2C_FREQUENCY
/** \brief
 * The initial I2C frequency.
 * \details
 * Defaults to 100000 Hz. \n
 * Standard frequencies: \n
 * 100000 Hz - Standard Mode \n
 * 400000 Hz - Fast Mode
 */
#define I2C_FREQUENCY 100000
#endif

#ifndef I2C_BUFFER_SIZE
/** \brief
 * The size of the buffer used for writes/target (slave) operations.
 * \details
 * Defaults to 32. If more than 32 bytes are needed for writes/target (slave) operations, increase. If more RAM is needed, decrease.
 */
#define I2C_BUFFER_SIZE 32
#elif I2C_BUFFER_SIZE >= 256
#error "I2C_BUFFER_SIZE is too big."
#endif

#ifndef I2C_BUS_BUSY_CHECKS
/** \brief
 * The amount of times the bus is checked before continuing with a read/write operation.
 * \details
 * Defaults to 16. Fixes design flaw where TWI hardware does not check if the bus has become busy during a stop interrupt,
 * so if multiple targets (slaves) receive the stop interrupt right before
 * they become the controller (master) and send a start, they all will think the bus is free and clobber each other.
 * Can be set to 0 when there is only one controller (master).
 * Increase if the game ever freezes.
 * More information: https://www.robotroom.com/Atmel-AVR-TWI-I2C-Multi-Master-Problem.html
 */
#define I2C_BUS_BUSY_CHECKS 16
#endif


#ifndef I2C_SCL_PIN
/** \brief
 * The pin on which the SCL line is connected.
 * \details
 * Defaults to PIND.
 */
#define I2C_SCL_PIN PIND
#endif

#ifndef I2C_SCL_BIT
/** \brief
 * The bit of the pin on which the SCL line is connected.
 * \details
 * Defaults to PIND0.
 */
#define I2C_SCL_BIT PIND0
#endif

#ifndef I2C_SDA_PIN
/** \brief
 * The pin on which the SDA line is connected.
 * \details
 * Defaults to PIND.
 */
#define I2C_SDA_PIN PIND
#endif

#ifndef I2C_SDA_BIT
/** \brief
 * The bit of the pin on which the sda line is connected.
 * \details
 * Defaults to PIND1.
 */
#define I2C_SDA_BIT PIND1
#endif

#ifdef __DOXYGEN__

/** \brief
 * The maxmimum number of players in the handshake/lobby.
 * \details
 * Cannot be more than I2C_MAX_ADDRESSES.
 */
#define I2C_MAX_PLAYERS

#endif


/** \brief
 * Error code used to mean success, returned by I2C::getTWError().
 * \details
 *  
 */
#define TW_SUCCESS 0xFF

/** \brief
 * Error code RETURNED by I2C::handshake, meaning the handshake has already been completed.
 * \details
 *  
 */
#define I2C_HANDSHAKE_FAILED 0xFE

/** \brief
 * The maximum amount of addresses available to a device.
 * \details
 * I2C uses a 7-bit addressing scheme with 127 available unique addresses.
 * However, addresses 0-7 and 120-127 are reserved by the standard and should not be used.
 * This leaves 112 addresses for devices to use.
 */
#define I2C_MAX_ADDRESSES 112

/** \brief
 * I2C library version.  
 * \details
 * For a given version x.y.z, the library version will be in the form xxxyyzz with no leading zeros on x.
 */
#define I2C_LIB_VER 10100

/** 
 * Provides all I2C functionality.
 */
class I2C {
public:
    /** \brief
     * Initalizes I2C hardware.
     * \details
     * This functions powers on, initializes, and sets up the clock on the TWI hardware.
     * Must be called after the arduboy hardware is initialized as `arduboy.boot()` disables the I2C(TWI) hardware.
     */
    static void init();

    /** \brief
     * Set the address of the device and enable/disable general calls on the I2C bus.
     * \param address The 7-bit address which to respond to.
     * Addresses 0-7 and 120-127 are reserved by the standard and should not be used.
     * \param generalCall Whether to enable or disable general calls. Defaults to false.
     * \note
     * General calls are a way for a device to broadcast data to every other device without addressing them individually.
     * They are sent by sending a write to address 0. If they are disabled, the device will not respond to them.
     */
    static void setAddress(uint8_t address, bool generalCall = false);

    /** \brief
     * Attempts to become the bus controller (master) and sends data over I2C to the specified address.
     * \param address The 7-bit address which to send the data. To send a general call, use address 0.
     * Addresses 1-7 and 120-127 are reserved by the standard and should not be used.
     * \param buffer A pointer to the data to send.
     * \param size The amount of data in bytes to send. This cannot be zero.
     * \param wait Whether or not to wait for the write to complete. If this is false, it will proceed with interrupts.
     * \details
     * \note
     * Sending general calls will only function if the `generalCall` argument of `setAddress` is true on every other device.
     * \note
     * Interally, this function uses a buffer to enable asynchronous writes. The buffer size is controlled by the macro `I2C_BUFFER_SIZE`
     * and defaults to 32. If the program needs to send more than 32 bytes at a time, `I2C_BUFFER_SIZE`
     * must be defined before including to be larger.
     * \see transmit() read()
     */
    static void write(uint8_t address, const void *buffer, uint8_t size, bool wait);
    
    template<typename T>
    static void write(uint8_t address, const T *buffer, bool wait);
    
    /** \brief
     * Attempts to become the bus controller (master) and reads data over I2C from the specified address.
     * \param address The 7-bit address which to receive the data from.
     * Addresses 0-7 and 120-127 are reserved by the standard and should not be used.
     * \param buffer A pointer to the buffer in which to store the data.
     * \param size The maximum amount of bytes to receive. This cannot be zero.
     * \note
     * Unlike the `write` function, this function is bufferless and is not limited to 32 bytes.
     * \see write()
     */
    static void read(uint8_t address, void *buffer, uint8_t size);
    
    template<typename T>
    static void read(uint8_t address, T *buffer);

    /** \brief
     * Transmits data back to the controller (master).
     * \param buffer A pointer to the data to send.
     * \param size The amount of the data in bytes to send.
     * \details
     * This function is intended to be called once inside the onRequest callback.
     * It fills the transmitting buffer with data to then be send one byte at a time.
     * If it is called multiple times, only the last call will be sent.
     * \note
     * Internally, this function uses a buffer. The buffer size is controlled by the macro `I2C_BUFFER_SIZE`
     * and defaults to 32. If the program needs to send more than 32 bytes at a time, `I2C_BUFFER_SIZE`
     * must be defined before including to be larger.
     * \see write() onRequest()
     */
    static void transmit(const void *buffer, uint8_t size);

    template <typename T>
    static void transmit(const T *object);
    
    /** \brief
     * Sets up the callback to be called when data is requested from the device's address (a read).
     * \param function The function to be called when data is requested.
     * \details
     * Example Callback and Usage:
     * \code{.cpp}
     * void dataRequest() {
     *   I2C::transmit(&players[id], 2);
     * }
     * ...
     * void setup() {
     *   ...
     *   I2C::onRequest(dataRequest);
     * }
     * \endcode
     * \note
     * To respond to the controller (master), use `transmit` instead of `write`.
     * \note
     * If no bytes are send in the callback, the result of the transmission is undefined.
     * \see onReceive() transmit() read()
     */
    static void onRequest(void (*function)());

    /** \brief
     * Sets up the callback to be called when data is sent to the device's address (a write)
     * \param function The function to be called when data is received.
     * \details
     * Example Callback and Usage:
     * \code{.cpp}
     * void dataReceive() {
     *   uint8_t *buffer = I2C::getBuffer();
     *   players[buffer[0]] = *(player_t *)buffer;
     * }
     * ...
     * void setup() {
     *   ...
     *   I2C::onReceive(dataReceive);
     * }
     * \endcode
     * \see onRequest() write()
     */
    static void onReceive(void (*function)());

    /** \brief
     * Gets the hardware error which happened in a previous read or write.
     * \return A byte indicating the error. TW_SUCCESS means no error has occurred.
     * The full list of error codes are available in the avr utils\twi.h.
     */
    static uint8_t getTWError();

    /** \brief
     * Gets a pointer to the I2C buffer holding received data.
     * \details
     * Intended to be used inside the onReceive callback.
     * \see onReceive()
     */
    static uint8_t *getBuffer();

    /** \brief
     * Gets the address from a provided id.
     * \param id An id between 0 and I2C_MAX_ADDRESSES - 1
     * \return The address corresponding to that id.
     * \details
     * This function is provided to standardize addresses for each id. It is used by I2C::handshake.
     */
    static uint8_t getAddressFromId(uint8_t id);
    /** \brief
     * Handshakes with other devices and returns a unique id once complete.
     * \return A unique id for this device.
     * \details
     * I2C_MAX_PLAYERS must be defined to 1 or more before including the header file to the number of players in the handshake.
     * This function will wait until every single player has joined.
     * 
     */
    static uint8_t handshake();

};

#ifdef I2C_IMPLEMENTATION
/** \brief
 * Not officially part of the library.
 */
namespace i2c_detail {

uint8_t           twiBuffer[I2C_BUFFER_SIZE];
volatile uint8_t *rxBuffer;
volatile uint8_t  bufferIdx;
volatile uint8_t  bufferSize;

volatile bool     active;
volatile uint8_t  slaRW;
volatile uint8_t  error;

void            (*onRequestFunction)();
void            (*onReceiveFunction)();

void stop() {
    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO) | _BV(TWEA);
    while (TWCR & _BV(TWSTO)) {  }
    i2c_detail::active = false;
}

#ifdef I2C_MAX_PLAYERS

#if I2C_MAX_PLAYERS > I2C_MAX_ADDRESSES
#error "Too many players. Max is I2C_MAX_ADDRESSES."
#endif // #if I2C_MAX_PLAYERS > I2C_MAX_ADDRESSES

volatile uint8_t handshakeState;

void handshakeOnReceive() {
    return;
}

void handshakeOnRequest() {
    handshakeState++;
    I2C::transmit(&handshakeState);
}
#endif // #ifdef I2C_MAX_PLAYERS

}

void I2C::init() {
    power_twi_enable();
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
    TWSR = 0; // clear prescaler bits
    TWBR = (F_CPU / I2C_FREQUENCY - 16) / 2;
}

void I2C::setAddress(uint8_t address, bool generalCall) {
    TWAR = address << 1 | generalCall;
}

void I2C::write(uint8_t address, const void *buffer, uint8_t size, bool wait) {
    while (i2c_detail::active) {}
    
    for (uint8_t i = 0; i < size; i++) {
        i2c_detail::twiBuffer[i] = ((const uint8_t *)buffer)[i];
    }
    i2c_detail::bufferIdx = 0;
    i2c_detail::bufferSize = size;

    i2c_detail::error = TW_SUCCESS;
    
    i2c_detail::active = true;
    i2c_detail::slaRW = address << 1 | TW_WRITE;

    uint8_t busyChecks = I2C_BUS_BUSY_CHECKS;
    while (busyChecks) {
        if ((I2C_SCL_PIN & _BV(I2C_SCL_BIT)) && (I2C_SDA_PIN & _BV(I2C_SDA_BIT))) {
            busyChecks--;
        } else {
            busyChecks = I2C_BUS_BUSY_CHECKS;
        }
    }

    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);
    if (wait) {
        while (i2c_detail::active) {}
    }
}

template<typename T>
void I2C::write(uint8_t address, const T *buffer, bool wait) {
    static_assert(sizeof(T) <= I2C_BUFFER_SIZE, "Size of T must be less than or equal to I2C_BUFFER_SIZE.");
    I2C::write(address, (const void *)buffer, sizeof(T), wait);
}

void I2C::read(uint8_t address, void *buffer, uint8_t size) {
    while (i2c_detail::active) {}
    
    i2c_detail::rxBuffer = (uint8_t *)buffer;

    i2c_detail::bufferIdx = 0;
    i2c_detail::bufferSize = size - 1;

    i2c_detail::error = TW_SUCCESS;

    i2c_detail::active = true;
    i2c_detail::slaRW = address << 1 | TW_READ;

    uint8_t busyChecks = I2C_BUS_BUSY_CHECKS;
    while (busyChecks) {
        if ((I2C_SCL_PIN & _BV(I2C_SCL_BIT)) && (I2C_SDA_PIN & _BV(I2C_SDA_BIT))) {
            busyChecks--;
        } else {
            busyChecks = I2C_BUS_BUSY_CHECKS;
        }
    }

    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);
    while (i2c_detail::active) {}
}

template<typename T>
void I2C::read(uint8_t address, T *object) {
    static_assert(sizeof(T) < 256, "Size of T must be less than 256.");
    I2C::read(address, (void *)object, sizeof(T));
}


void I2C::transmit(const void *buffer, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        i2c_detail::twiBuffer[i] = ((uint8_t *)buffer)[i];
    }
    i2c_detail::bufferIdx = 0;
    i2c_detail::bufferSize = size;
}

template <typename T>
void I2C::transmit(const T *object) {
    static_assert(sizeof(T) <= I2C_BUFFER_SIZE, "Size of T must be less than or equal to I2C_BUFFER_SIZE.");
    I2C::transmit((const void *)object, sizeof(T));
}

void I2C::onRequest(void (*function)()) {
    i2c_detail::onRequestFunction = function;
}
void I2C::onReceive(void (*function)()) {
    i2c_detail::onReceiveFunction = function;
}

inline uint8_t I2C::getTWError() {
    return i2c_detail::error;
}

inline uint8_t *I2C::getBuffer() {
    return i2c_detail::twiBuffer;
}

#ifdef I2C_MAX_PLAYERS

inline uint8_t I2C::getAddressFromId(uint8_t id) {
    return 0x8 + id;
}

uint8_t I2C::handshake() {
    for (int8_t i = I2C_MAX_PLAYERS - 1; i >= 0; ) {
        uint8_t dummy;

        I2C::read(I2C::getAddressFromId(i), &dummy, 1);

        switch (I2C::getTWError()) {
        case TW_MR_SLA_NACK:
            I2C::setAddress(I2C::getAddressFromId(i), true);
            I2C::onReceive(i2c_detail::handshakeOnReceive);
            I2C::onRequest(i2c_detail::handshakeOnRequest);

            // handshakeState is the number of times the callback has been called.
            // When the callback has been called i times, the final Arduboy has joined.
            while (i2c_detail::handshakeState < i) { }

            return i;
        case TW_SUCCESS:
            i--;
            break;
        }
    }
    return I2C_HANDSHAKE_FAILED;
}

#endif

ISR(TWI_vect, ISR_NAKED) {
    asm volatile (
R"(
; --------------------- defines ----------------------- ;

.equ TWCR, 0xBC
.equ TWSR, 0xB9
.equ TWDR, 0xBB

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
; r18 - TWSR (never used after function call)
; r19 - general use
; r30 - general use
; r31 - general use
; --------------------- prologue ---------------------- ;
push r18
in r18, __SREG__
push r18
push r30
push r31
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
; save and restore tmp and zero registers (could be used in function calls)
push __tmp_reg__
push __zero_reg__
clr __zero_reg__
; ----------------------------------------------------- ;

; switch (TWSR)
lds r18, TWSR ; no mask needed because prescaler bits are cleared

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
    ; TWDR = i2c_detail::slaRW;
    lds r30, %[slaRW]
    sts TWDR, r30
    ; TWCR = REPLY_NACK;
    ldi r30, REPLY_NACK
    sts TWCR, r30
    ; return;
    rjmp pop_reti

TW_MT_SLA_ACK:
TW_MT_DATA_ACK:
    ; if (i2c_detail::bufferIdx >= bufferSize) { stop(); return; }
    lds r30, %[bufferIdx]
    lds r31, %[bufferSize]
    cp r30, r31
    
    brlt 1f ; 64 instruction limit on branches
    rjmp stop_reti
    1:

    ; TWDR = i2c_detail::twiBuffer[i2c_detail::bufferIdx++];
    ; Increment bufferIdx but preserve r30
    inc r30
    sts %[bufferIdx], r30
    dec r30

    ; Use SUBI and SBCI as (non-existant) ADDI and (non-existant) ADCI
    clr r31
    subi r30, lo8(-(%[twiBuffer]))
    sbci r31, hi8(-(%[twiBuffer]))
    ld r30, Z
    sts TWDR, r30

    ; https://ww1.microchip.com/downloads/en/DeviceDoc/ATmega48APA88APA168APA328P-SiliConErrataClarif-DS80000855A.pdf
    ; Section 2.3.1
    lpm ; 3 cycle delay

    ; TWCR = REPLY_NACK;
    ldi r30, REPLY_NACK
    sts TWCR, r30
    ; return;
    rjmp pop_reti

TW_MT_ARB_LOST:
    ; TWCR = REPLY_ACK;
    ldi r30, REPLY_ACK
    sts TWCR, r30
    ; i2c_detail::error = TW_MT_ARB_LOST;
    ldi r30, 0x38
    sts %[error], r30
    ; active = false;
    ; return;
    rjmp active_false_reti
; ----------------------------------------------------- ;

TW_MR_DATA_NACK:
TW_MR_DATA_ACK:
    ; i2c_detail::rxBuffer[i2c_detail::bufferIdx++] = TWDR;
    lds r30, %[bufferIdx]
    inc r30
    sts %[bufferIdx], r30
    dec r30

    ; Use SUBI and SBCI as (non-existant) ADDI and (non-existant) ADCI
    clr r31
    subi r30, lo8(-(%[rxBuffer]))
    sbci r31, hi8(-(%[rxBuffer]))
    lds r19, TWDR
    st Z, r19
    
    ; if (TWSR == TW_MR_DATA_NACK) { stop(); return; }
    ; r18 holds TWSR
    cpi r18, 0x58
    brne 1f ; 64 instruction limit on branches
    rjmp stop_reti
    1:
; ------------------ fallthrough ---------------------- ;
TW_MR_SLA_ACK:
    ; if (i2c_detail::bufferIdx < i2c_detail::bufferSize) {
    ;    TWCR = REPLY_ACK;
    ; } else {
    ;    TWCR = REPLY_nACK;
    ; }
    ; return;

    lds r30, %[bufferIdx]
    lds r31, %[bufferSize]
    cp r30, r31
    ldi r30, REPLY_ACK
    brlt 1f
    ldi r30, REPLY_NACK
    1:
    sts TWCR, r30
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
    ; i2c_detail::active = TWSR; (true)
    sts %[active], r18 ; r18 holds TWSR
    ; i2c_detail::bufferIdx = 0;
    sts %[bufferIdx], __zero_reg__
    ; TWCR = REPLY_ACK;
    ldi r30, REPLY_ACK
    sts TWCR, r30
    ; return;
    rjmp pop_reti

TW_SR_DATA_ACK:
TW_SR_GCALL_DATA_ACK:
    ; i2c_detail::twiBuffer[i2c_detail::bufferIdx++] = TWDR;
    lds r30, %[bufferIdx]
    inc r30
    sts %[bufferIdx], r30
    dec r30

    ; Use SUBI and SBCI as (non-existant) ADDI and (non-existant) ADCI
    clr r31
    subi r30, lo8(-(%[twiBuffer]))
    sbci r31, hi8(-(%[twiBuffer]))
    lds r19, TWDR
    st Z, r19

    ; TWCR = REPLY_ACK;
    ldi r30, REPLY_ACK
    sts TWCR, r30
    ; return;
    rjmp pop_reti
TW_SR_STOP:
    ; TWCR = REPLY_ACK;
    ldi r30, REPLY_ACK
    sts TWCR, r30
    ; i2c_detail::onReceiveFunction();
    lds r30, %[onReceiveFunction]
    lds r31, %[onReceiveFunction] + 1
    icall
    ; i2c_detail::active = false;
    ; return;
    rjmp active_false_reti;

; ----------------------------------------------------- ;
TW_ST_ARB_LOST_SLA_ACK:
TW_ST_SLA_ACK:
    ; i2c_detail::active = TWSR; (true)
    sts %[active], r18
    ; i2c_detail::onRequestFunction();
    lds 30, %[onRequestFunction]
    lds 31, %[onRequestFunction] + 1
    icall
; ------------------ fallthrough ---------------------- ;
TW_ST_DATA_ACK:
    ; TWDR = i2c_detail::twiBuffer[i2c_detail::bufferIdx++];
    lds r30, %[bufferIdx] 
    inc r30
    sts %[bufferIdx], r30
    dec r30
    ; Use SUBI and SBCI as (non-existant) ADDI and (non-existant) ADCI
    clr r31
    subi r30, lo8(-(%[twiBuffer]))
    sbci r31, hi8(-(%[twiBuffer]))
    ld r30, Z
    sts TWDR, r30
    
    ; if (i2c_detail::bufferIdx < i2c_detail::bufferSize) {
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
    ldi r30, REPLY_ACK
    sts TWCR, r30
    ; i2c_detail::active = false;
    ; return;
    rjmp active_false_reti
; ----------------------------------------------------- ;
default:
    ; i2c_detail::error = TWSR;
    sts %[error], r18 

    stop_reti:

    ; TWCR = STOP;
    ldi r30, STOP
    sts TWCR, r30

    ; while (TWCR & _BV(TWSTO)) {}
    1:
    lds r30, TWCR
    sbrc r30, TWSTO ; skip if bit in register clear
    rjmp 1b

    active_false_reti:
    ; i2c_detail::active = false;
    sts %[active], __zero_reg__

; --------------------- epilogue ---------------------- ;
    pop_reti:
    pop __zero_reg__
    pop __tmp_reg__
    pop r27
    pop r26
    pop r25
    pop r24
    pop r23
    pop r22
    pop r21
    pop r20
    pop r19
    pop r31
    pop r30
    pop r18
    out __SREG__, r18
    pop r18
    reti
)"
        : // Output Operands
        [error]            "=m" (i2c_detail::error),
        [active]           "=m" (i2c_detail::active),
        [bufferIdx]        "=m" (i2c_detail::bufferIdx),
        [rxBuffer]         "=m" (i2c_detail::rxBuffer),
        [twiBuffer]        "=m" (i2c_detail::twiBuffer)
        : // Input Operands
        [onRequestFunction] "m" (i2c_detail::onRequestFunction),
        [onReceiveFunction] "m" (i2c_detail::onReceiveFunction),
        [bufferSize]        "m" (i2c_detail::bufferSize),
        [slaRW]             "m" (i2c_detail::slaRW)
    );
}

#if 0
ISR(TWI_vect) {
    switch (TWSR) { // prescaler bits are cleared, no mask needed
    case TW_START:
        TWDR = i2c_detail::slaRW;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
        break;
    // MT
    case TW_MT_SLA_ACK:
    case TW_MT_DATA_ACK:
        if (i2c_detail::bufferIdx < i2c_detail::bufferSize) {
            TWDR = i2c_detail::twiBuffer[i2c_detail::bufferIdx++];
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
        } else {
            i2c_detail::stop();
        }
        break;
    case TW_MT_ARB_LOST: // same as TW_MR_ARB_LOST
        i2c_detail::active = false;
        i2c_detail::error = TW_MT_ARB_LOST;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;
    // MR
    case TW_MR_DATA_ACK:
        i2c_detail::rxBuffer[i2c_detail::bufferIdx++] = TWDR;
        __attribute__((fallthrough));
    case TW_MR_SLA_ACK:
        if (i2c_detail::bufferIdx < i2c_detail::bufferSize) {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        } else {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
        }
        break;
    case TW_MR_DATA_NACK:
        i2c_detail::rxBuffer[i2c_detail::bufferIdx++] = TWDR;
        i2c_detail::stop();
        break;
    // ST
    case TW_ST_SLA_ACK:
    case TW_ST_ARB_LOST_SLA_ACK:
        i2c_detail::active = true;
        i2c_detail::onRequestFunction();
        __attribute__((fallthrough));
    case TW_ST_DATA_ACK:
        TWDR = i2c_detail::twiBuffer[i2c_detail::bufferIdx++];
        if (i2c_detail::bufferIdx < i2c_detail::bufferSize) {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        } else {
            TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
        }
        break;
    case TW_ST_DATA_NACK:
    case TW_ST_LAST_DATA: // last interrupt cleared TWEA
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        i2c_detail::active = false;
        break;
    // SR
    case TW_SR_SLA_ACK:
    case TW_SR_GCALL_ACK:
    case TW_SR_ARB_LOST_SLA_ACK:
    case TW_SR_ARB_LOST_GCALL_ACK:
        i2c_detail::bufferIdx = 0;
        i2c_detail::active = true;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;
    case TW_SR_GCALL_DATA_ACK:
    case TW_SR_DATA_ACK:
        i2c_detail::twiBuffer[i2c_detail::bufferIdx++] = TWDR;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;
    case TW_SR_STOP:
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        i2c_detail::onReceiveFunction(i2c_detail::twiBuffer);
        i2c_detail::active = false;
        break;
    default:
        i2c_detail::error = TWSR;
        i2c_detail::stop();
        break;
    }
}
#endif // #if 0

#endif // #ifdef I2C_IMPLEMENTATION