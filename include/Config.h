#pragma once

// PIN Allocation
// ESP32-S3...
// https://www.luisllamas.es/en/esp32-s3-hardware-details-pinout/
// https://www.phippselectronics.com/the-esp32-s3-devkitc-1-pinouts/

// Pins listed in physical order

// PIN_xx_Dyy_Az
// xx = pin number
// yy = digital input number (for our use, doesn't relate to device h/w)
// z  = analog input number (for our use, doesn't relate to device h/w)

// Comments key:
// [ ... ] - general space to show what this pin is used for
// OK = OK for us to use this pin for our general use. First priority GPIO ports usable after a system boot - no special setup is required.
// ok = still good to use but maybe implications (e.g. no pull up/down resistor available)
// No OK or ok - don't recommend using this pin - either important for device itself, or it can be a pain to use/has implications
// pud = Pull Up/Down resistor present

// Notes:
// Some pins can be used at the expense of more general device functionality - so we attempt to not interfear with anything critical (e.g. so USB doesn't stop working!)
// Do not use ADC2 pins for ADC - device ADC2 has issues (especially if using wireless/bluetooth) and is easier just to not use it than faff around
// GPIO00->21 have internal pull-up/down resistors, can also be woken up from deep low-power mode.
// GPIO22->25 are not available
// GPIO26->48 input only with no pull-up/down resistors
// GPIO26->32 usually used for SPI flash and PSRAM, not recommended for other uses
// GPIO33->37 not recommended in ESP32-S3R8 / ESP32-S3R8V if using SPI or PSRAM (usually not using these)
// GPIO39->42 cannot be used if using JTAG debugging

// Pins listed Top to Bottom (as if physically looking at device with CPU at top and USB sockets at the bottom)
// Pins are numbered as printed on device

// // SIDE ONE
// //      +3v3
// //      +3v3
// //      RST
// #define PIN_04_D01_A1 4     // [      ] OK pud ADC1_3 Touch_01
// #define PIN_05_D02_A2 5     // [      ] OK pud ADC1_4 Touch_02
// #define PIN_06_D03_A3 6     // [      ] OK pud ADC1_5 Touch_03
// #define PIN_07_D04_A4 7     // [      ] OK pud ADC1_6 Touch_04
// #define PIN_15_D05    15    // [      ] OK pud adc2_4          - Do not use for ADC
// #define PIN_16_D06    16    // [      ] OK pud adc2_5          - Do not use for ADC
// #define PIN_17_D07    17    // [      ] OK pud adc2_6          - Do not use for ADC
// #define PIN_18_D08    18    // [      ] OK pud adc2_7          - Do not use for ADC
// #define PIN_08_D09    8     // [      ] OK pud ADC1_7 Touch_08 - I2C SDA default
// //      PIN_3               //             pud ADC1_2 Touch_03 - Boot Strapping pin (JTAG signal source) - DO NOT USE
// //      PIN_46              //                                 - Boot Strapping pin (Chip boot mode and ROM messages printing), input only, no internal pull up/down - DO NOT USE
// #define PIN_09_D10_A5 9     // [      ] OK pud ADC1_8 Touch_09 - I2C SCL Default
// #define PIN_10_D11_A6 10    // [      ] OK pud ACD1_9 Touch_10 -                      SPI3 CS
// #define PIN_11_D12    11    // [      ] OK pud adc2_0 Touch_11 - Do not use for ADC - SPI3 MOSI
// #define PIN_12_D13    12    // [      ] OK pud adc2_1 Touch_12 - Do not use for ADC - SPI3 CLK
// #define PIN_13_D14    13    // [      ] OK pud adc2_2 Touch_13 - Do not use for ADC - SPI3 MISO
// #define PIN_14_D15    14    // [      ] OK pud adc2_3 Touch_14 - Do not use for ADC
// //      +5v in              //                                 - +5v from USB if IN-OUT jumper bridged
// //      Gnd

// // SIDE TWO
// //      Gnd
// //      TX            43    //                        - UART0 TX/Debug
// //      RX            44    //                        - UART0 RX/Debug
// #define PIN_01_D16_A7 1     // [      ] OK pud ADC1_0 Touch_01
// #define PIN_02_D17_A8 2     // [      ] OK pud ADC1_1 Touch_02
// #define Pin_42_D18    18    // [      ] ok                     - JTAG MTMS
// #define Pin_41_D19    19    // [      ] ok                     - JTAG MTDI
// #define Pin_40_D20    40    // [Oled  ] ok                     - JTAG MTDO
// #define Pin_39_D21    39    // [Oled  ] ok                     - JTAG MTCK, SPI2 CS
// #define Pin_38_D22    38    // [Neopix] ok
// //      PIN_37              // [      ] ok                     - SPI2 MISO - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
// //      PIN_36              // [      ] ok                     - SPI2 CLK  - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
// //      PIN_35              // [      ] ok                     - SPI2 MOSI - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
// //      PIN_0               //                                 - Boot Strapping Pin Boot Mode
// //      PIN_45              //                                 - Boot Strapping Pin VDD SPI Voltage (VDD_SPI voltage, selects between 1.8v and 3.3v)
// #define PIN_48_D23    48    // [      ] ok
// #define PIN_47_D24    47    // [      ] ok
// #define PIN_21_D25    21    // [      ] OK pud
// //      PIN_20              //                                 - USB_D+ - if reconfigured as normal GPIO, USB-JTAG functionality unavailable - i.e. don't expect USB to work! - DO NOT USE
// //      PIN 19              //                                 - USB_D- - DO NOT USE
// //      Gnd
// //      Gnd

//#include "Structs.h"

#define SETUP_DELAY     250           // Delay between stages of set up. Can be pretty much instant, but having a delay gives a chance to see pretty things on screen.

//#define EXTRA_SERIAL_DEBUG          // Enable to print loads of extra information to serial
//#define INCLUDE_BENCHMARKS          // Includes some basic performance statistics of running device over serial
#define SHOW_FPS                    // Shows FPS top left of screen
//#define WHITE_SCREEN                // Display will show a solid white screen, handy when physically aligning panel in device

// XBox 360 X-Plorer Guitar Hero Controller (model 95065)
// HID/XInput variant of BLE Gamepad https://github.com/Mystfit/ESP32-BLE-CompositeHID
// USB\VID_1430&PID_4748 and Compatible Id of USB\MS_COMP_XUSB10 (to tell the driver that it's a wired xbox 360 controller).
#define VID 1430
#define PID 4748

//#define SERIAL_SPEED 115200
#define SERIAL_SPEED 921600

#define SubSecondCount          30  // Number of sub-samples to take across a second when calculating clicks per second counts. Higher number = more accurate but more overhead.
#define ThrottledUpdatesRate 0.016  // Fraction of a second to wait between throttled updates - 0.016 = 16ms = ~60FPS

#define CLEAR_STATS_ON_FLIP // Resets stats counter when screen flipped (just a handy way for a manual zeroing without needing an extra button)

// Some standard values
#define LOW 0x0
#define HIGH 0x1
#define NONE 0

#define ClearColour ST7735_BLACK