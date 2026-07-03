/*
MIT License

Copyright (c) 2024-2026 sub1inear

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including but not limited to the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom this Software is
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
#include <ArduboyI2C.h>

Arduboy2 arduboy;

// -------------------------------------------------------------
// Constants
// -------------------------------------------------------------
constexpr uint8_t fieldLeft = 0;
constexpr uint8_t fieldRight = WIDTH;
constexpr uint8_t fieldTop = 0;
constexpr uint8_t fieldBottom = HEIGHT;
constexpr uint8_t fieldCenterX = (fieldLeft + fieldRight) / 2;
constexpr uint8_t fieldCenterY = (fieldTop + fieldBottom) / 2;
constexpr uint8_t fieldWidth = fieldRight - fieldLeft;
constexpr uint8_t fieldHeight = fieldBottom - fieldTop;

constexpr uint8_t paddleWidth = 2;
constexpr uint8_t paddleHeight = 10;
constexpr int8_t paddleSpeed = 2;
constexpr int8_t paddleStartY = fieldCenterY - paddleHeight / 2;

constexpr uint8_t ballSize = 2;
constexpr uint8_t ballSpeedX = 1;
constexpr uint8_t ballSpeedY = 1;
constexpr uint8_t ballStartX = fieldCenterX - ballSize / 2;
constexpr uint8_t ballStartY = fieldCenterY - ballSize / 2;

// 5x7 font + 1 pixel spacing
constexpr uint8_t charWidth = 6;

constexpr uint8_t leftScoreX = fieldCenterX - (fieldCenterX / 2) - (charWidth / 2);
constexpr uint8_t leftScoreY = 0;
constexpr uint8_t rightScoreX = fieldCenterX + (fieldCenterX / 2) - (charWidth / 2);
constexpr uint8_t rightScoreY = 0;

constexpr uint8_t nullInput = 0xFF;

// -------------------------------------------------------------
// Structs
// -------------------------------------------------------------

struct Ball {
    int8_t x;
    int8_t y;
    int8_t dx;
    int8_t dy;
};

struct Player {
    int8_t paddleY;
    uint8_t score;
};

// -------------------------------------------------------------
// Data
// -------------------------------------------------------------

Ball ball;

Player leftPlayer;
Player rightPlayer;

// right player is the controller (master) and left player is the target (slave)
bool isRightPlayer = false;

bool serveRight = true;
bool serveDown = false;

// volatile variables are used to communicate between the main loop and the I2C callbacks
// otherwise the compiler may optimize the variables away in while (variable) { } loops
// because it can't see that they will change

// have we received input from the controller (master) yet?
volatile bool controllerReceived = false;
// the corresponding input
volatile uint8_t controllerInput = 0;

// have we been requested to send our input to the controller (master) yet?
volatile bool controllerRequested = false;
// the corresponding input
volatile uint8_t targetInput = 0;

// -------------------------------------------------------------
// Target Functions
// -------------------------------------------------------------

void onReceive() {
    const uint8_t *buffer = I2C::getBuffer();
    // the first byte of the buffer is the input from the remote player
    controllerInput = buffer[0];
    // tell the main loop that we have new input
    controllerReceived = true;
}

void onRequest() {
    // send our input to the controller (master)
    I2C::reply(targetInput);
    // tell the main loop that this has happened
    controllerRequested = true;
}

// -------------------------------------------------------------
// Reset Functions
// -------------------------------------------------------------

void resetBall() {
    // reset ball position
    ball.x = ballStartX;
    ball.y = ballStartY;

    // alternate the direction of the ball each time it is reset

    ball.dx = serveRight ? ballSpeedX : -ballSpeedX;
    serveRight = !serveRight;

    ball.dy = serveDown ? ballSpeedY : -ballSpeedY;
    serveDown = !serveDown;
}


void resetPlayers() {
    // reset player positions and scores
    leftPlayer = { paddleStartY, 0 };
    rightPlayer = { paddleStartY, 0 };
}

void reset() {
    resetPlayers();
    resetBall();
}

// -------------------------------------------------------------
// Update Functions
// -------------------------------------------------------------

void updatePlayer(Player &player, uint8_t input) {
    // update player position based on input
    // `UP_BUTTON` and `DOWN_BUTTON` correspond with `arduboy.buttonsState()` values,
    // which we used to get the input
    if (input & UP_BUTTON) {
        player.paddleY -= paddleSpeed;
    }
    if (input & DOWN_BUTTON) {
        player.paddleY += paddleSpeed;
    }
    // constrain the player position to the field boundaries
    // player is drawn from the top left corner, so we need to subtract the paddle height from the bottom boundary
    player.paddleY = constrain(player.paddleY, fieldTop, fieldBottom - paddleHeight);
}

void updateBall() {
    // update ball position based on its velocity
    ball.x += ball.dx;
    ball.y += ball.dy;

    // if the ball leaves the field boundaries, ...
    if (ball.y <= fieldTop || ball.y >= fieldBottom - ballSize) {
        // reverse its vertical direction
        ball.dy = -ball.dy;
        // constrain its position to the field boundaries
        ball.y  = constrain(ball.y, fieldTop, fieldBottom - ballSize);
    }

    Rect ballRect(ball.x, ball.y, ballSize, ballSize);

    Rect leftPaddleRect(fieldLeft, leftPlayer.paddleY, paddleWidth, paddleHeight);

    // if the ball is moving left and collides with the left paddle, ...
    // (if the ball is moving right, it can't collide with the left paddle, so we can skip the collision check)
    if (ball.dx < 0 && arduboy.collide(leftPaddleRect, ballRect)) {
        // reverse its horizontal direction
        ball.dx = -ball.dx;
        // constrain its position to the field boundaries
        ball.x = fieldLeft + paddleWidth;
    }
    Rect rightPaddleRect(fieldRight - paddleWidth, rightPlayer.paddleY, paddleWidth, paddleHeight);

    // if the ball is moving right and collides with the right paddle, ...
    // (if the ball is moving left, it can't collide with the right paddle, so we can skip the collision check)
    if (ball.dx > 0 && arduboy.collide(rightPaddleRect, ballRect)) {
        // reverse its horizontal direction
        ball.dx = -ball.dx;
        // constrain its position to the field boundaries
        ball.x = fieldRight - ballSize - paddleWidth;
    }

    // if the ball touches the left side, ...
    if (ball.x < fieldLeft) {
        // increment the right player's score,
        // but only if it is less than 255, to avoid overflow
        if (rightPlayer.score < UINT8_MAX) {
            rightPlayer.score++;
        }
        resetBall();
    // if the ball touches the right side, ...
    } else if (ball.x > fieldRight - ballSize) {
        // increment the left player's score,
        // but only if it is less than 255, to avoid overflow
        if (leftPlayer.score < UINT8_MAX) {
            leftPlayer.score++;
        }
        resetBall();
    }
}

void update(uint8_t leftInput, uint8_t rightInput) {
    updatePlayer(leftPlayer, leftInput);
    updatePlayer(rightPlayer, rightInput);
    updateBall();
}

// -------------------------------------------------------------
// Draw Functions
// -------------------------------------------------------------

void drawScore(uint8_t score, int8_t x, int8_t y) {
    arduboy.setCursor(x, y);
    arduboy.print(score);
}

void drawScores() {
    drawScore(leftPlayer.score, leftScoreX, 0);
    drawScore(rightPlayer.score, rightScoreX, 0);
}

void drawPlayer(const Player &player, int8_t x) {
    arduboy.fillRect(x, player.paddleY, paddleWidth, paddleHeight);
}

void drawPlayers() {
    drawPlayer(leftPlayer, fieldLeft);
    // rects are drawn from the top left corner,
    // so we need to subtract the paddle width from the right boundary
    drawPlayer(rightPlayer, fieldRight - paddleWidth);
}

void drawBall() {
    arduboy.fillRect(ball.x, ball.y, ballSize, ballSize);
}

void drawCenterLine() {
    for (uint8_t y = fieldTop; y < fieldBottom; y += 4) {
        arduboy.drawFastVLine(fieldCenterX, y, 2);
    }
}

void draw() {
    arduboy.clear();

    drawScores();

    drawCenterLine();

    drawPlayers();

    drawBall();

    arduboy.display();
}

void drawMessage(const __FlashStringHelper *message) {
    // __FlashStringHelper ensures message is stored in flash memory (with the F() macro)
    // otherwise the exactly the same as `char`
    arduboy.clear();
    arduboy.print(message);
    arduboy.display();
}

// -------------------------------------------------------------
// Main Functions
// -------------------------------------------------------------

void setup() {
    arduboy.begin();
    I2C::begin();

    // check if the cable is flipped and waits for it to be flipped back
    // only needed on the FX-C, as the Arduboy Mini does not have a way to flip the cable
    I2C::checkCableFlipped([]() {
        // cable is flipped, draw message
        // F() macro ensures the string is stored in flash memory instead of RAM
        drawMessage(F("Please flip the cable\non this device."));
    });

    isRightPlayer = I2C::handshake([]() {
        // waiting for a handshake, display message
        // F() macro ensures the string is stored in flash memory instead of RAM
        drawMessage(F("Waiting for other\nplayer..."));
    });

    // if we're the target (slave), ...
    if (!isRightPlayer) {
        // set up the I2C callbacks
        I2C::onReceive(onReceive);
        I2C::onRequest(onRequest);
        // set our address to the null address
        // so we won't respond to any requests until we want to
        I2C::setAddress(I2C_NULL_ADDRESS);
    }

    reset();
}

void loop() {
    if (!arduboy.nextFrame()) {
        return;
    }
    uint8_t localInput = arduboy.buttonsState();
    uint8_t leftInput, rightInput;

    if (isRightPlayer) {
        rightInput = localInput;
        // send our input to the other device and wait for it to be received
        do {
            I2C::write(I2C_TARGET_ADDRESS, rightInput, true);
        } while (I2C::getError() != I2C_ERROR_NONE);
        // read the other device's input and wait for it to be received
        do {
            I2C::read(I2C_TARGET_ADDRESS, leftInput);
        } while (I2C::getError() != I2C_ERROR_NONE);
    } else {
        // let the onReceive() callback get our input through targetInput
        targetInput = leftInput = localInput;
        // set our address to I2C_TARGET_ADDRESS; we're ready
        I2C::setAddress(I2C_TARGET_ADDRESS);
        // wait for the controller (master) to send us its input
        while (!controllerReceived) { }
        // store the controller's input and reset the flag
        rightInput = controllerInput;
        controllerReceived = false;

        // wait for the controller (master) to request our input
        while (!controllerRequested) { }
        // reset the flag
        controllerRequested = false;
        // set our address to the null address; we're done
        // otherwise the controller may get ahead of us and request our input again before we have a chance to update it
        // thus destroying our synchronization
        I2C::setAddress(I2C_NULL_ADDRESS);
    }

    update(leftInput, rightInput);
    draw();
}
