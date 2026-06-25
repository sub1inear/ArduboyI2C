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
#include <Arduboy2.h>
#define I2C_IMPLEMENTATION
#include "ArduboyI2C.h"

#undef assert
#define assert(cond, a, b) do { \
		if (!(cond)) { \
			Serial.print(F("Assertion failed: " #cond ", ")); \
			Serial.println(a); \
			Serial.print(F(", ")); \
			Serial.println(b); \
			return false; \
		} \
	} while (0)
#define asserteq(a, b) do { \
		if ((a) != (b)) { \
			Serial.print(F("Assertion failed: " #a " == " #b ", ")); \
			Serial.print(a); \
			Serial.print(F(", ")); \
			Serial.println(b); \
			return false; \
		} \
	} while (0)
#define assertcb(cond, var, assigna, assignb) do { \
		if (!(cond)) { \
			var = F(#cond); \
			assigna; \
			assignb; \
		} \
	} while (0)
#define assertcb_s(var, a, b) do { \
	    if (var) { \
			Serial.print(F("Assertion failed: ")); \
			Serial.println(var); \
			Serial.print(F(", ")); \
			Serial.print(a); \
			Serial.print(F(", ")); \
			Serial.println(b); \
			return false; \
		} \
	} while (0)


constexpr uint8_t numPlayers = 2;
constexpr uint8_t bufferExpected[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

Arduboy2 arduboy;

volatile bool readCallbackCalled = false;

volatile bool writeCallbackCalled = false;
const __FlashStringHelper *volatile writeCallbackError = nullptr;
volatile int writeCallbackA = 0;
volatile uint8_t writeCallbackB = 0;

void displayTest(const __FlashStringHelper *name, bool result) {
	arduboy.print(name);
	arduboy.print(F(": "));
	if (result) {
		arduboy.println(F("PASS"));
	} else {
		arduboy.println(F("FAIL"));
	}
	arduboy.display();
}

bool testBegin() {
	I2C::begin();
	asserteq(TWCR, (_BV(TWEN) | _BV(TWIE) | _BV(TWEA)));
	asserteq(TWAR, 0);
	asserteq(TWSR, 248); // 0b11111000 -> TW idle, prescaler = 0
	asserteq(TWBR, (F_CPU / I2C_FREQUENCY - 16) / 2);
	asserteq((I2C_PORT & (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT))), (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT)));
	asserteq((I2C_DDR & (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT))), 0);
	return true;
}

bool testSetAddress() {
	I2C::setAddress(0x12, false);
	asserteq(TWAR, 0x12 << 1);
	I2C::setAddress(0x12, true);
	asserteq(TWAR, (0x12 << 1 | 1));
	return true;
}

void testWriteCallback(const uint8_t *buffer, uint8_t size) {
	writeCallbackCalled = true;
	assertcb(size == 16, writeCallbackError, writeCallbackA = size, writeCallbackB = 16);

	int memcmpResult = memcmp(buffer, bufferExpected, 16);
	assertcb(memcmpResult == 0, writeCallbackError, writeCallbackA = memcmpResult, writeCallbackB = 0);
}

bool testWrite(uint8_t id) {
	if (id == 0) {
		I2C::write(I2C::idToAddress(1), bufferExpected, 16, true);
		asserteq(I2C::getError(), TW_SUCCESS);
	} else {
		uint32_t start = millis();
		while (!writeCallbackCalled) {
			if (millis() - start > 1000) {
				return false;
			}
		}
		assertcb_s(writeCallbackError, writeCallbackA, writeCallbackB);
	}
	return true;
}

void testReadCallback() {
	I2C::reply(bufferExpected, 8);
	I2C::reply(bufferExpected + 8, 8);
	readCallbackCalled = true;
}

bool testRead(uint8_t id) {
	if (id == 0) {
		uint8_t buffer[16];
		I2C::read(I2C::idToAddress(1), buffer);
		asserteq(I2C::getError(), TW_SUCCESS);
		asserteq(memcmp(buffer, bufferExpected, 16), 0);
	} else {
		uint32_t start = millis();
		while (!readCallbackCalled) {
			if (millis() - start > 1000) {
				return false;
			}
		}
	}
	return true;
}

bool testCheckEmulator() {
	bool isEmulator = I2C::checkEmulator();
	asserteq(isEmulator, false);
	return true;
}

bool testCheckCableFlipped() {
	I2C::checkCableFlipped([]() {});
	return true;
}

bool testIdToAddress() {
	for (uint8_t i = 0; i <= I2C_MAX_IDS - 1; i++) {
		asserteq(I2C::idToAddress(i), 0x8 + i);
	}
	return true;
}

bool testHandshake(uint8_t id) {
	assert(id != I2C_HANDSHAKE_FULL, id, I2C_HANDSHAKE_FULL);
	assert(id < numPlayers, id, numPlayers);
	return true;
}

void setup() {
    arduboy.begin();
	arduboy.clear();

	displayTest(F("begin"), testBegin());
	displayTest(F("setAddress"), testSetAddress());
	displayTest(F("ccFlipped"), testCheckCableFlipped());

	uint8_t id = I2C::handshake(numPlayers);
	I2C::onReceive(testWriteCallback);
	I2C::onRequest(testReadCallback);
	displayTest(F("handshake"), testHandshake(id));

	displayTest(F("read"), testRead(id));
	displayTest(F("write"), testWrite(id));
	displayTest(F("checkEmulator"), testCheckEmulator());
	displayTest(F("idToAddress"), testIdToAddress());

	while (true) { }
}

void loop() {
}