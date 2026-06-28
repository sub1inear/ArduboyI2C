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
// define in one file before including
#define I2C_IMPLEMENTATION
// disable cable flipping and software pullups for the Arduboy Mini
// #define I2C_PLATFORM I2C_PLATFORM_MINI
#include <ArduboyI2C.h>

Arduboy2 arduboy;

struct Player {
    int8_t x;
    int8_t y;
};

Player localPlayer = { 0, 0 };
Player remotePlayer = { 0, 0 };

// unique id for this device
uint8_t id = 0;

void displayMessage(const __FlashStringHelper *message) {
    // __FlashStringHelper ensures message is stored in flash memory (with the F() macro)
    // otherwise the exactly the same as `char`
    arduboy.clear();
    arduboy.print(message);
    arduboy.display();
}

void onReceive() {
    const uint8_t *buffer = I2C::getBuffer();
    // interpret the received data as a player struct
    const Player *newPlayer = reinterpret_cast<const Player *>(buffer);
    // update remote player data
    remotePlayer = *newPlayer;
}

void setup() {
    // initialize arduboy hardware
    arduboy.begin();

    // initialize I2C (twi) hardware
    I2C::begin();

    // if an emulator without I2C support is detected, ...
    if (I2C::checkEmulator()) {
        // display a message
        displayMessage(F("Emulator does not\nsupport I2C."));
        // wait forever
        while (true) { }
    }

    // check if the cable is flipped
    // calls function to display message if it is flipped
    // waits for it to be flipped back
    // only needed on the FX-C, as the Arduboy Mini does not have a way to flip the cable
    I2C::checkCableFlipped([]() {
        // cable is flipped, display message
        displayMessage(F("Please flip the cable\non this device."));
    });

    // display handshaking message, I2C::handshake blocks
    displayMessage(F("Waiting for other\nplayer..."));

    // handshake with other devices and get a unique id for this device
    // note: I2C::handshake enables general calls by default
    id = I2C::handshake();

    // if the handshaking is full (two players have joined already), exit
    if (id == I2C_HANDSHAKE_FULL) {
        arduboy.exitToBootloader();
    }

    // setup our receive event to be called when we receive a write
    I2C::onReceive(onReceive);
}

void loop() {
    // wait for next frame
    if (!arduboy.nextFrame()) {
        return;
    }
    arduboy.clear();

    // move our player around with the D-Pad
    if (arduboy.pressed(RIGHT_BUTTON)) { localPlayer.x++; }
    if (arduboy.pressed(LEFT_BUTTON))  { localPlayer.x--; }
    if (arduboy.pressed(DOWN_BUTTON))  { localPlayer.y++; }
    if (arduboy.pressed(UP_BUTTON))    { localPlayer.y--; }

    localPlayer.x = constrain(localPlayer.x, 0, WIDTH - 8);
    localPlayer.y = constrain(localPlayer.y, 0, HEIGHT - 8);

    // send out a general call to give the other device our data
    // false -> will not wait for the write to complete
    I2C::write(I2C_GENERAL_CALL_ADDR, localPlayer, false);

    // draw the players
    // id 0 -> filled, id 1 -> outlined
    Player &filledPlayer = (id == 0) ? localPlayer : remotePlayer;
    Player &outlinedPlayer = (id == 0) ? remotePlayer : localPlayer;

    arduboy.fillRect(filledPlayer.x, filledPlayer.y, 8, 8, WHITE);
    arduboy.drawRect(outlinedPlayer.x, outlinedPlayer.y, 8, 8, WHITE);

    // display
    arduboy.display();
}
