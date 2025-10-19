#pragma once

#include "Config.h"
#include "Structs.h"
#include <Adafruit_SH110X.h>

void FlipScreen(Input* input);

// Screen width/height in pixels
#define SCREEN_WIDTH 128
#define HALF_SCREEN_WIDTH 64
#define QUARTER_SCREEN_WIDTH 32
#define SCREEN_HEIGHT 64

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL)
// ESP32-S3: 8(SDA), 9(SCL) - can any GPIO pin to implement the I2C protocol using the command wire.begin(SDA, SCL) [https://www.luisllamas.es/en/esp32-i2c/]
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//Adafruit_SSD1306 Display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
extern Adafruit_SH1106G Display;

#define C_WHITE SH110X_WHITE
#define C_BLACK SH110X_BLACK
// #define C_WHITE SSD1306_WHITE
// #define C_BLACK SSD1306_BLACK