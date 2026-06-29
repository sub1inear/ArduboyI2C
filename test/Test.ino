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
#define I2C_PLATFORM I2C_PLATFORM_FX_C
#include <ArduboyI2C.h>

#undef assert
#define assert(cond, a, b) do { \
		if (!(cond)) { \
			Serial.print(F("Assertion failed: " #cond ", ")); \
			Serial.println(a); \
			Serial.print(F(", ")); \
			Serial.println(b); \
			allTestsPassed = false; \
			return false; \
		} \
	} while (0)
#define assert_eq(a, b) do { \
		if ((a) != (b)) { \
			Serial.print(F("Assertion failed: " #a " == " #b ", ")); \
			Serial.print(a); \
			Serial.print(F(", ")); \
			Serial.println(b); \
			allTestsPassed = false; \
			return false; \
		} \
	} while (0)
#define assert_cb(cond, var, assigna, assignb) do { \
		if (!(cond)) { \
			var = F(#cond); \
			assigna; \
			assignb; \
		} \
	} while (0)
#define assert_cb_ok(var, a, b) do { \
	    if (var) { \
			Serial.print(F("Assertion failed: ")); \
			Serial.print(var); \
			Serial.print(F(", ")); \
			Serial.print(a); \
			Serial.print(F(", ")); \
			Serial.println(b); \
			allTestsPassed = false; \
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

bool allTestsPassed = true;

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

bool testBeginEnd() {
	I2C::begin();
	assert_eq(PRR0 & _BV(PRTWI), 0);
	assert_eq(TWCR, (_BV(TWEN) | _BV(TWIE) | _BV(TWEA)));
	assert_eq(TWSR, 248); // 0b11111000 -> TW idle, prescaler = 0
	assert_eq(TWBR, (F_CPU / I2C_FREQUENCY - 16) / 2);
	assert_eq((I2C_PORT & (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT))), (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT)));
	assert_eq((I2C_DDR & (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT))), 0);
	I2C::end();
	assert_eq(TWCR, 0);
	assert_eq(PRR0 & _BV(PRTWI), _BV(PRTWI));
	assert_eq((I2C_PORT & (_BV(I2C_SDA_BIT) | _BV(I2C_SCL_BIT))), 0);
	I2C::begin();
	return true;
}

bool testSetAddress() {
	I2C::setAddress(0x12, false);
	assert_eq(TWAR, 0x12 << 1);
	I2C::setAddress(0x12, true);
	assert_eq(TWAR, (0x12 << 1 | 1));
	return true;
}

void testWriteCallback() {
	writeCallbackCalled = true;
	assert_cb(I2C::getBufferSize() == 16, writeCallbackError, writeCallbackA = I2C::getBufferSize(), writeCallbackB = 16);

	int memcmpResult = memcmp(I2C::getBuffer(), bufferExpected, 16);
	assert_cb(memcmpResult == 0, writeCallbackError, writeCallbackA = memcmpResult, writeCallbackB = 0);
}

bool testWrite(uint8_t id) {
	if (id == 0) {
		I2C::write(I2C::idToAddress(1), bufferExpected, 16, true);
		assert_eq(I2C::getError(), I2C_ERROR_NONE);
	} else {
		uint32_t start = millis();
		while (!writeCallbackCalled) {
			if (millis() - start > 1000) {
				Serial.println(F("Write callback timeout"));
				return false;
			}
		}
		assert_cb_ok(writeCallbackError, writeCallbackA, writeCallbackB);
	}
	return true;
}

void testReadCallback() {
	I2C::reply(bufferExpected);
	readCallbackCalled = true;
}

bool testRead(uint8_t id) {
	if (id == 0) {
		uint8_t buffer[16];
		I2C::read(I2C::idToAddress(1), buffer);
		assert_eq(I2C::getError(), I2C_ERROR_NONE);
		assert_eq(memcmp(buffer, bufferExpected, 16), 0);
	} else {
		uint32_t start = millis();
		while (!readCallbackCalled) {
			if (millis() - start > 1000) {
				Serial.println(F("Read callback timeout"));
				return false;
			}
		}
	}
	return true;
}

bool testCheckEmulator() {
	bool isEmulator = I2C::checkEmulator();
	assert_eq(isEmulator, false);
	return true;
}

bool testCheckCableFlipped() {
	I2C::checkCableFlipped([]() {});
	return true;
}

bool testIdToAddress() {
	for (uint8_t i = 0; i <= I2C_MAX_IDS - 1; i++) {
		assert_eq(I2C::idToAddress(i), 0x8 + i);
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

	displayTest(F("beginEnd"), testBeginEnd());
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

	if (allTestsPassed) {
		arduboy.digitalWriteRGB(GREEN_LED, RGB_ON);
	} else {
		arduboy.digitalWriteRGB(RED_LED, RGB_ON);
	}

}

void loop() {
	if (arduboy.anyPressed(A_BUTTON | B_BUTTON)) {
		arduboy.exitToBootloader();
	}
}
