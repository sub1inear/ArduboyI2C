#include <Arduboy2.h>
// define in one file before including
#define I2C_IMPLEMENTATION
// declare the number of players in the handshake
// cannot be greater than I2C_MAX_ADDRESSES
#define I2C_MAX_PLAYERS 4
#include "ArduboyI2C.h"

Arduboy2 arduboy;

struct player_t {
    // this is used to identify our message as belonging to us
    uint8_t id;
    // general data
    uint8_t x;
    uint8_t y;
};
// player data array
player_t players[I2C_MAX_PLAYERS];

// stores unique id from 0 to I2C_MAX_PLAYERS - 1
uint8_t id;

void onReceive() {
    uint8_t *buffer = I2C::getBuffer();
    // copy data to buffer[0] (id)
    players[buffer[0]].x = buffer[1];
    players[buffer[0]].y = buffer[2];
}
// main functions
void setup() {
    // initialize arduboy hardware
    arduboy.begin();
    // initialize I2C(twi) hardware
    I2C::init();

    // arduboy.clear();
    // arduboy.print("Waiting for other\nplayers...");
    // arduboy.display();
    // get unique id and wait for other players to join
    // Note: I2C::handshake enables general calls by default
    id = I2C::handshake();

    // if the handshake has been completed (I2C_MAX_PLAYERS has been reached), exit
    if (id == I2C_HANDSHAKE_COMPLETED) {
        arduboy.exitToBootloader();
    }
    // setup our rx event to be called when we receive a writeTo
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
    I2C::writeTo(0x00, &players[id], false);
    // draw all of the players
    for (uint8_t i = 0; i < I2C_MAX_PLAYERS; i++) {
        arduboy.fillRect(players[i].x, players[i].y, 8, 8);
    }
    // display
    arduboy.display();
}