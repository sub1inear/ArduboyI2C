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
#include <ArduboyI2C.h>

Arduboy2 arduboy;

struct Player {
    int8_t x;
    int8_t y;
};

Player localPlayer = { 0, 0 };
Player remotePlayer = { 0, 0 };

I2C::Role role;

void drawMessage(const __FlashStringHelper *message) {
    // __FlashStringHelper ensures message is stored in flash memory (with the F() macro)
    // otherwise the exactly the same as `char`
    arduboy.clear();
    arduboy.print(message);
    arduboy.display();
}

void onReceive() {
    // copy the received data into the remote player struct
    remotePlayer = *reinterpret_cast<const Player *>(I2C::getBuffer());
}

void onRequest() {
    // send our player data to the other device
    I2C::reply(localPlayer);
}

void setup() {
    // initialize arduboy hardware
    arduboy.begin();

    // initialize I2C hardware
    I2C::begin();

    // check if the cable is flipped and wait for it to be flipped back
    I2C::checkCableFlipped([]() {
        // cable is flipped, draw message
        // F() macro ensures the string is stored in flash memory instead of RAM
        drawMessage(F("Please flip the cable\non this device."));
    });

    // waits for other device and determines if this device is the controller (master) or target (slave)
    role = I2C::handshake([]() {
        // waiting for a handshake, display message
        // F() macro ensures the string is stored in flash memory instead of RAM
        drawMessage(F("Waiting for other\nplayer..."));
    });

    // if we're the target (slave), set up the receive and request callbacks
    if (role == I2C::Role::Target) {
        I2C::onReceive(onReceive);
        I2C::onRequest(onRequest);
    }
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

    // limit our player to the screen bounds
    // drawn from the top left corner, so the max x/y is the screen width/height minus the player size (8x8)
    localPlayer.x = constrain(localPlayer.x, 0, WIDTH - 8);
    localPlayer.y = constrain(localPlayer.y, 0, HEIGHT - 8);

    // if we're the controller (master), ...
    if (role == I2C::Role::Controller) {
        // read the remote player data from the other device
        I2C::read(I2C::targetAddress, remotePlayer);
        // send our player data to the other device
        // no point in waiting for the write to complete, so we send asynchronously
        I2C::write(I2C::targetAddress, localPlayer, I2C::Mode::Async);
    } /* else {
        // if we're the target (slave), onRequest() and onReceive() will send and receive our data
    } */

    // draw the players
    // controller -> filled, target -> outlined
    Player &filledPlayer = (role == I2C::Role::Controller) ? localPlayer : remotePlayer;
    Player &outlinedPlayer = (role == I2C::Role::Controller) ? remotePlayer : localPlayer;

    arduboy.fillRect(filledPlayer.x, filledPlayer.y, 8, 8, WHITE);
    arduboy.drawRect(outlinedPlayer.x, outlinedPlayer.y, 8, 8, WHITE);

    arduboy.display();
}
