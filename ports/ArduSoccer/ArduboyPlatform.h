#pragma once

#include <avr/pgmspace.h>
#include <Arduboy2.h>

#include "Platform.h"

extern Arduboy2Base arduboy;

inline void clearDisplay(uint8_t colour)
{
	uint8_t data = colour ? 0xff : 0;
	uint8_t* ptr = arduboy.sBuffer;
	int count = 128 * 64 / 8;
	while(count--)
		*ptr++ = data;
	//memset(_displayBuffer, data, LCDWIDTH * LCDHEIGHT / 8);
}


constexpr uint8_t deviceIdNull = 0xff;

class ArduboyPlatform : public PlatformBase
{
public:
	void playSound(const uint16_t* sound);
	bool connectMultiplayer();
	void disconnectMultiplayer();

	void update();
	void updateInput();

	uint8_t deviceId = deviceIdNull;
	uint8_t data;
	volatile bool dataAvailable = false;
};

void ERROR(const char* msg);

extern ArduboyPlatform Platform;

inline void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t w, uint8_t h, uint8_t color)
{
	arduboy.drawBitmap(x, y, bitmap, w, h, color);
}

inline void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t colour)
{
	arduboy.fillRect(x, y, w, h, colour);
}



