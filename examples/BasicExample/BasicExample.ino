#include <Arduboy2.h>
// define in one file before including
#define I2C_IMPLEMENTATION
#include "ArduboyI2C.h"

constexpr uint8_t numPlayers = 2;

Arduboy2 arduboy;

struct player_t {
    // this is used to identify our message as belonging to us
    uint8_t id;
    // general data
    uint8_t x;
    uint8_t y;
};
// player data array
player_t players[numPlayers];

// stores unique id from 0 to numPlayers - 1
uint8_t id;

void onReceive(const uint8_t *buffer, uint8_t size) {
    // interpret the received data as a player struct
    player_t *newPlayer = (player_t *)buffer;

    // copy everything except the id (it is only used to identify the message)
    player_t *curPlayer = &players[newPlayer->id];
    curPlayer->x = newPlayer->x;
    curPlayer->y = newPlayer->y;
}
// main functions
void setup() {
    // initialize arduboy hardware
    arduboy.begin();
    // initialize I2C (twi) hardware
    I2C::init();

    arduboy.clear();
    arduboy.print("Waiting for other\nplayers...");
    arduboy.display();

    // get unique id and wait for other players to join
    // note: I2C::handshake enables general calls by default
    id = I2C::handshake(numPlayers);

    // if the handshaking failed (numPlayers has been reached already), exit
    if (id == I2C_HANDSHAKE_FAILED) {
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
    I2C::write(I2C_GENERAL_CALL, players[id], false);
    // draw all of the players
    for (uint8_t i = 0; i < numPlayers; i++) {
        arduboy.fillRect(players[i].x, players[i].y, 8, 8);
    }
    // display
    arduboy.display();
}