#pragma once

#include "Config.h"
#include "Defines.h"
#include "Structs.h"
// #include <Adafruit_SH110X.h>
#include <LovyanGFX.hpp>

void FlipScreen(Input *input);

// Screen width/height in pixels
#define SCREEN_WIDTH 128
#define SCREEN_MEMORY_WIDTH 132
#define HALF_SCREEN_WIDTH 64
#define QUARTER_SCREEN_WIDTH 32
#define SCREEN_HEIGHT 64
#define SCREEN_MEMORY_HEIGHT 64
#define SCREEN_X_OFFSET 2
#define SCREEN_Y_OFFSET 0

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL)
// ESP32-S3: 8(SDA), 9(SCL) - can any GPIO pin to implement the I2C protocol using the command wire.begin(SDA, SCL) [https://www.luisllamas.es/en/esp32-i2c/]
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
// Adafruit_SSD1306 Display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// extern Adafruit_SH1106G Display;

class LGFX : public lgfx::LGFX_Device
{
public:
    LGFX();

private:
    // Physical panel
    // Panel_GC9A01
    // Panel_GDEW0154M09
    // Panel_HX8357B
    // Panel_HX8357D
    // Panel_ILI9163
    // Panel_ILI9341
    // Panel_ILI9342
    // Panel_ILI9481
    // Panel_ILI9486
    // Panel_ILI9488
    // Panel_IT8951
    // Panel_RA8875
    // Panel_SH110x - SH1106, SH1107 - PanelWidth=128, MemoryWidth=132, XOffset=2
    // Panel_SSD1306
    // Panel_SSD1327
    // Panel_SSD1331
    // Panel_SSD1351 - SSD1351, SSD1357
    // Panel_SSD1963
    // Panel_ST7735
    // Panel_ST7735S
    // Panel_ST7789
    // Panel_ST7796
    lgfx::Panel_SH110x _panel_instance;

    // Bus mechanism
    // Bus_SPI
    // Bus_I2C
    // Bus_Parallel8
    lgfx::Bus_I2C _bus_instance;

    lgfx::Light_PWM _light_instance;

    // Touch
    // Touch_CST816S
    // Touch_FT5x06 - FT5206, FT5306, FT5406, FT6206, FT6236, FT6336, FT6436
    // Touch_GSL1680E_800x480 - GSL_1680E, 1688E, 2681B, 2682B
    // Touch_GSL1680F_800x480
    // Touch_GSL1680F_480x272
    // Touch_GSLx680_320x320
    // Touch_GT911
    // Touch_STMPE610
    // Touch_TT21xxx - TT21100
    // Touch_XPT2046
    lgfx::Touch_FT5x06 _touch_instance;
};

extern LGFX Display;

// Adafruit
// #define C_WHITE SH110X_WHITE
// #define C_BLACK SH110X_BLACK

// LovyanGFX
#define C_WHITE 0xFFFF
#define C_BLACK 0x0000

// #define C_WHITE SSD1306_WHITE
// #define C_BLACK SSD1306_BLACK