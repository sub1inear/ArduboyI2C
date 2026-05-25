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
// disable the built-in handshake functionality (we're implementing our own)
#define I2C_USE_HANDSHAKE 0
#include "ArduboyI2C.h"

/**
 * Lobby overview:
 * - Each device finds its unique id by reading I2C addresses.
 *     - If a device reads an address with no response, it takes that address as its id and starts responding to it.
 *     - If a device does respond, it reads its state. If it is in the game state, it continues to the game. This allows late joiners.
 * - In the lobby, every device continually broadcasts its id using the I2C general call.
 *     - Devices keep a short timeout for every id they hear from.
 *     - A timeout greater than 0 is considered active.
 *     - Timeouts start at 0.
 *     - The lobby displays the number of active players out of 4.
 *     - If there are 2 or more active players, a message to press A to start is displayed.
 * - Pressing A sends a special broadcast that tells everyone to start.
 * - In the game, each device continually broadcasts its data using the I2C general call.
 * - The timeout system still applies, so if a player disconnects (or didn't join at all), their character will not be drawn.
 */

constexpr uint8_t maxPlayers = 4;
constexpr uint8_t nullId = 0xFF; // signals the start of the game; cannot be used as a player id
constexpr uint8_t activeTimeout = 100; // frames until a player is considered inactive after not receiving their data

Arduboy2 arduboy;

enum class State : uint8_t {
    Lobby,
    Game
};

// stores the current state (lobby or game) of this device
// must be volatile because it is changed in the callback
volatile State state = State::Lobby;

struct Player {
    // used to identify our message as belonging to us
    uint8_t id;
    // general data
    uint8_t x = 0;
    uint8_t y = 0;
    // lobby data
    uint8_t timeout = 0;
};
// player data array
// must be volatile because it is changed in the callback
volatile Player players[maxPlayers];

// stores unique id from 0 to maxPlayers - 1
uint8_t id;

// --------------------- helper functions ---------------------

void displayMessage(const __FlashStringHelper *message) {
    // __FlashStringHelper ensures message is stored in flash memory (with the F() macro)
    arduboy.clear();
    arduboy.print(message);
    arduboy.display();
}

void startGame() {
    state = State::Game;
    I2C::onReceive(gameOnReceive);
}

void broadcastLobbyPing() {
    // send out a general call with our id to let everyone know we're here
    I2C::write(I2C_GENERAL_CALL, id, false);
}

void broadcastStartGame() {
    // send out a general call with nullId to tell everyone to start
    I2C::write(I2C_GENERAL_CALL, nullId, false);
}

uint8_t runTimeout() {
    // starts at 0 (we include ourself in the player count)
    uint8_t numActivePlayers = 0;
    for (uint8_t i = 0; i < maxPlayers; i++) {
        // if this player is active (including ourself)
        if (players[i].timeout > 0) {
            // decrease their timeout
            players[i].timeout--;
            // count them as active
            numActivePlayers++;
        }
    }
    return numActivePlayers;
}

// --------------------- callback functions ---------------------

void handshakeOnReceive(const uint8_t *buffer, uint8_t size) {
    // buffer[0] is the player id
    uint8_t otherPlayerId = buffer[0];
    if (otherPlayerId == nullId) {
        // nullId is the message to start the game
        startGame();
    } else {
        // otherwise, update the timeout of the id sent
        players[otherPlayerId].timeout = activeTimeout;
    }
}

void gameOnReceive(const uint8_t *buffer, uint8_t size) {
    // interpret the received data as a player struct
    Player *newPlayer = (Player *)buffer;

    // copy everything except the id (it is only used to identify the message)
    Player *curPlayer = &players[newPlayer->id];
    curPlayer->x = newPlayer->x;
    curPlayer->y = newPlayer->y;
    curPlayer->timeout = activeTimeout;
}

void onRequest() {
    // give our current state to the device that is handshaking with us
    // lets the device know if it should join the lobby or the game
    I2C::transmit(state);
}

// --------------------- lobby functions ---------------------

void startLobby() {
    I2C::onRequest(onRequest);

    // loop through each player
    for (uint8_t i = 0; i < maxPlayers; ) {
        // read their state from the address corresponding to the player id
        State otherPlayerState;
        uint8_t address = I2C::getAddressFromId(i);
        I2C::read(address, otherPlayerState);
        switch (I2C::getTWError()) {
        case TW_MR_SLA_NACK: // no response
            // take this id
            id = i;
            players[i].id = i;

            // set our timeout so we know we're here!
            players[i].timeout = activeTimeout;

            // register handshake callback
            // (before setting addressto avoid race condition)
            I2C::onReceive(handshakeOnReceive);

            // set our address to this address
            I2C::setAddress(address, true);
            return;
        case TW_SUCCESS: // response received
            // if they are in the game, join the game
            if (otherPlayerState == State::Game) {
                startGame();
                return;
            }
            // otherwise, try the next id
            i++;
            break;
        default: // some other error occurred
            // retry reading this id
            // no increment of i
            break;
        }
    }
    // if we exit the loop without returning, all ids have responded
    // so lobby is full
    displayMessage(F("Lobby is full."));
    while (true) { }
}

void runLobby(uint8_t numActivePlayers) {
    // poll buttons (needed only in this branch for arduboy.justPressed())
    arduboy.pollButtons();

    // print lobby message (e.g. "2/4 players joined")
    arduboy.print(numActivePlayers);
    arduboy.print(F("/4 players joined.\n"));

    // if the lobby is not empty,
    if (numActivePlayers > 1) {
        // let the players know to press A to start
        arduboy.print("Press A to start.");
        // if A is pressed,
        if (arduboy.justPressed(A_BUTTON)) {
            // send out a general call with  nullId to tell everyone to start
            I2C::write(I2C_GENERAL_CALL, nullId, false);
            // start the game for ourselves
            startGame();
            // exit early to prevent sending out another lobby message
            return;
        }
    }
    broadcastLobbyPing();
}

// --------------------- game functions ---------------------

void runGame() {
    // move our player around with the D-Pad
    players[id].x += arduboy.pressed(RIGHT_BUTTON) - arduboy.pressed(LEFT_BUTTON);
    players[id].y += arduboy.pressed(DOWN_BUTTON) - arduboy.pressed(UP_BUTTON);

    // send out a general call to give every other device our data
    I2C::write(I2C_GENERAL_CALL, players[id], false);
    // draw all of the active players
    for (uint8_t i = 0; i < maxPlayers; i++) {
        // if this player is active (timeout is not zero), draw them
        if (players[i].timeout > 0) {
            arduboy.fillRect(players[i].x, players[i].y, 8, 8);
        }
    }
}

// --------------------- main functions ---------------------

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

    // start the lobby (blocking)
    startLobby();
}

void loop() {
    // wait for next frame
    if (!arduboy.nextFrame()) {
        return;
    }
    // clear
    arduboy.clear();

    // decrease timeouts and get number of active players
    uint8_t numActivePlayers = runTimeout();

    switch (state) {
    case State::Lobby:
        runLobby(numActivePlayers);
        break;
    case State::Game:
        runGame();
        break;
    }

    // display
    arduboy.display();
}
