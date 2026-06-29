#define I2C_IMPLEMENTATION
#define I2C_PLATFORM I2C_PLATFORM_FX_C
#include <ArduboyI2C.h>
#include "ArduboyPlatform.h"
#include "Engine.h"

//#include "Generated/Data_Audio.h"

ArduboyPlatform Platform;

void onReceive()
{
	const uint8_t *buffer = I2C::getBuffer();
	Platform.data = buffer[0];
	Platform.dataAvailable = true;
}

void ArduboyPlatform::updateInput()
{
	lastInputState[LOCAL_PLAYER] = inputState[LOCAL_PLAYER];
	inputState[LOCAL_PLAYER] = 0;

	if(arduboy.pressed(A_BUTTON))
	{
		inputState[LOCAL_PLAYER] |= Input_Btn_A;
	}
	if(arduboy.pressed(B_BUTTON))
	{
		inputState[LOCAL_PLAYER] |= Input_Btn_B;
	}
	if(arduboy.pressed(UP_BUTTON))
	{
		inputState[LOCAL_PLAYER] |= Input_Dpad_Up;
	}
	if(arduboy.pressed(DOWN_BUTTON))
	{
		inputState[LOCAL_PLAYER] |= Input_Dpad_Down;
	}
	if(arduboy.pressed(LEFT_BUTTON))
	{
		inputState[LOCAL_PLAYER] |= Input_Dpad_Left;
	}
	if(arduboy.pressed(RIGHT_BUTTON))
	{
		inputState[LOCAL_PLAYER] |= Input_Dpad_Right;
	}
}

void ArduboyPlatform::update()
{
    if(arduboy.audio.enabled() != !m_isMuted)
    {
		if(m_isMuted)
		{
				arduboy.audio.off();
		}
		else
		{
				arduboy.audio.on();
		}
    }
	updateInput();
	if (deviceId != deviceIdNull)
	{
		if (deviceId == 0)
		{
			sendInput();
			getRemoteInput();
		}
		else
		{
			getRemoteInput();
			sendInput();
		}
	}
}

void ArduboyPlatform::disconnectMultiplayer()
{
	deviceId = deviceIdNull;
	I2C::onReceive(nullptr);
}

bool ArduboyPlatform::connectMultiplayer()
{
	arduboy.display(CLEAR_BUFFER);

	if (I2C::checkEmulator())
	{
		engine.renderer.drawText(smallFont, PSTR("NO I2C IN EMULATOR"), 12, 30, 1);
		arduboy.display();
		while (true) {}
	}

	I2C::checkCableFlipped([]() {
		engine.renderer.drawText(smallFont, PSTR("PLEASE FLIP THE CABLE"), 2, 30, 1);
		arduboy.display(CLEAR_BUFFER);
	});

	engine.renderer.drawText(smallFont, PSTR("WAITING FOR PLAYERS"), 8, 30, 1);
	arduboy.display(CLEAR_BUFFER);
	deviceId = I2C::handshake();
	if (deviceId == I2C_HANDSHAKE_FULL)
	{
		arduboy.exitToBootloader();
	}

	I2C::onReceive(onReceive);
	return deviceId == 0;
}


void ArduboyPlatform::sendInput()
{
	do
	{
		I2C::write(I2C_GENERAL_CALL_ADDR, inputState[LOCAL_PLAYER], true);
	}
	while (I2C::getError() != I2C_ERROR_NONE);
}

void ArduboyPlatform::getRemoteInput()
{
	while (!dataAvailable)
	{
		// Wait for data to be received
	}
	dataAvailable = false;

	lastInputState[REMOTE_PLAYER] = inputState[REMOTE_PLAYER];
	inputState[REMOTE_PLAYER] = data;
}
