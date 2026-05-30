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
#include "ArduboyI2C.h"

constexpr uint8_t numPlayers = 2;

Arduboy2 arduboy;

struct Player {
    // used to identify our message as belonging to us
    uint8_t id;
    // general data
    uint8_t x;
    uint8_t y;
};
// player data array
Player players[numPlayers];

// stores unique id from 0 to numPlayers - 1
uint8_t id;

void displayMessage(const __FlashStringHelper *message) {
    // __FlashStringHelper ensures message is stored in flash memory (with the F() macro)
    arduboy.clear();
    arduboy.print(message);
    arduboy.display();
}

void onReceive(const uint8_t *buffer, uint8_t size) {
    // interpret the received data as a player struct
    Player *newPlayer = (Player *)buffer;

    // copy everything except the id (it is only used to identify the message)
    Player *curPlayer = &players[newPlayer->id];
    curPlayer->x = newPlayer->x;
    curPlayer->y = newPlayer->y;
}
// main functions
void setup() {
    // initialize arduboy hardware
    arduboy.begin();

    // initialize I2C (twi) hardware
    I2C::begin();

    // if an emulator without I2C support is detected, ...
    if (I2C::detectEmulator()) {
        // display a message
        displayMessage(F("Emulator does not\nsupport I2C."));
        // wait forever
        while (true) { }
    }

    // check if the cable is flipped
    // calls function to display message if it is flipped
    // only needed on the FX-C, as the Arduboy Mini does not have a way to flip the cable
    I2C::checkCableFlipped([]() {
        // display cable flipped message
        displayMessage(F("Please flip the cable\non this device."));
    });

    // display handshaking message,
    // I2C::handshake blocks
    displayMessage(F("Waiting for other\nplayer..."));

    // get unique id and wait for other players to join
    // note: I2C::handshake enables general calls by default
    id = I2C::handshake(numPlayers);

    // if the handshaking is full (numPlayers has been reached already), exit
    if (id == I2C_HANDSHAKE_FULL) {
        arduboy.exitToBootloader();
    }
    // setup our receive event to be called when we receive a write
    I2C::onReceive(onReceive);
    // identify our player data packets
    players[id].id = id;
}

void loop() {
    // wait for next frame
    if (!arduboy.nextFrame()) {
        return;
    }
    // clear screen
    arduboy.clear();
    // move our player around with the D-Pad
    players[id].x += arduboy.pressed(RIGHT_BUTTON) - arduboy.pressed(LEFT_BUTTON);
    players[id].y += arduboy.pressed(DOWN_BUTTON) - arduboy.pressed(UP_BUTTON);

    // send out a general call to give every other device our data
    // false -> will not wait for the write to complete
    I2C::write(I2C_GENERAL_CALL, players[id], false);
    // draw all of the players
    for (uint8_t i = 0; i < numPlayers; i++) {
        arduboy.fillRect(players[i].x, players[i].y, 8, 8);
    }
    // display
    arduboy.display();
}
