#pragma once

#include "Config.h"
#include "Defines.h"

#define DEVICE_NAME "Humbug"    // Max 17 characters

// SIDE ONE
//      +3v3
//      +3v3
//      RST
#define PIN_04_D01_A1 4     // [Whammy] OK pud ADC1_3 Touch_01
#define PIN_05_D02_A2 5     // [Hat1 R] OK pud ADC1_4 Touch_02
#define PIN_06_D03_A3 6     // [Hat1 L] OK pud ADC1_5 Touch_03
#define PIN_07_D04_A4 7     // [Blue  ] OK pud ADC1_6 Touch_04
#define PIN_15_D05    15    // [Green ] OK pud adc2_4           - Do not use for ADC
#define PIN_16_D06    16    // [Orange] OK pud adc2_5           - Do not use for ADC
#define PIN_17_D07    17    // [Yellow] OK pud adc2_6           - Do not use for ADC
#define PIN_18_D08    18    // [Red   ] OK pud adc2_7           - Do not use for ADC
#define PIN_08_D09    8     // [      ] OK pud ADC1_7 Touch_08  - I2C SDA default
//      PIN_3               //             pud ADC1_2 Touch_03  - Boot Strapping pin (JTAG signal source) - DO NOT USE
//      PIN_46              //                                  - Boot Strapping pin (Chip boot mode and ROM messages printing), input only, no internal pull up/down - DO NOT USE
#define PIN_09_D10_A5 9     // [      ] OK pud ADC1_8 Touch_09  - I2C SCL Default
#define PIN_10_D11_A6 10    // [Tilt  ] OK pud ACD1_9 Touch_10  -                      SPI3 CS
#define PIN_11_D12    11    // [      ] OK pud adc2_0 Touch_11  - Do not use for ADC - SPI3 MOSI 
#define PIN_12_D13    12    // [      ] OK pud adc2_1 Touch_12  - Do not use for ADC - SPI3 CLK
#define PIN_13_D14    13    // [Start ] OK pud adc2_2 Touch_13  - Do not use for ADC - SPI3 MISO
#define PIN_14_D15    14    // [Select] OK pud adc2_3 Touch_14  - Do not use for ADC
//      +5v in              //                                  - +5v from USB if IN-OUT jumper bridged
//      Gnd

// SIDE TWO
//      Gnd
//      TX            43    //                                  - UART0 TX/Debug
//      RX            44    //                                  - UART0 RX/Debug
#define PIN_01_D16_A7 1     // [      ] OK pud ADC1_0 Touch_01
#define PIN_02_D17_A8 2     // [      ] OK pud ADC1_1 Touch_02
#define PIN_42_D18    42    // [      ] ok                      - JTAG MTMS
#define PIN_41_D19    41    // [FlipSc] ok                      - JTAG MTDI
#define PIN_40_D20    40    // [Screen] ok                      - JTAG MTDO
#define PIN_39_D21    39    // [Screen] ok                      - JTAG MTCK, SPI2 CS
#define PIN_38_D22    38    // [Neopix] ok 
//      PIN_37              // [      ] ok                      - SPI2 MISO - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
//      PIN_36              // [      ] ok                      - SPI2 CLK  - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
//      PIN_35              // [      ] ok                      - SPI2 MOSI - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
//      PIN_0               //                                  - Boot Strapping Pin Boot Mode
//      PIN_45              //                                  - Boot Strapping Pin VDD SPI Voltage (VDD_SPI voltage, selects between 1.8v and 3.3v)
#define PIN_48_D23    48    // [      ] ok  NEOPIXEL
#define PIN_47_D24    47    // [      ] ok
#define PIN_21_D25    21    // [      ] OK pud 
//      PIN_20              //                                  - USB_D+ - if reconfigured as normal GPIO, USB-JTAG functionality unavailable - i.e. don't expect USB to work! - DO NOT USE
//      PIN 19              //                                  - USB_D- - DO NOT USE
//      Gnd
//      Gnd

// All possible points ordered into Blocks/connectors
// Documented wire color in <brackets> - used in prototype

// Battery monitor ... 10k -> +3.3, 20k -> Gnd
#define PIN_BATTERYMONITOR PIN_02_D17_A8  // Battery Voltage - 20K ohm to Gnd + 10K ohm to +ve

// Guitar Neck Buttons
#define Hat1_Right PIN_05_D02_A2  // [05] <Gray>
#define Hat1_Left PIN_06_D03_A3   // [06] <Brown>
#define B_Blue PIN_07_D04_A4      // [07] <Blue> 4th
#define B_Green PIN_15_D05        // [15] <Green> 1st
#define B_Orange PIN_16_D06       // [16] <Orange> 5th
#define B_Yellow PIN_17_D07       // [17] <Yellow> 3rd
// Gnd                            // [G ] <black> - Set as Gnd
#define B_Red PIN_18_D08          // [18] <Red> 2nd - CONNECT TO PIN 18

// Extra buttons
#define B_Tilt PIN_10_D11_A6      // [10] < > Tilt Sensor
// 11 Spare?                      // [11] <White>
// 12 Spare?                      // [12] <Black>

// End Buttons
#define B_Start PIN_13_D14        // [13] <Red>
#define B_Select PIN_14_D15       // [14] <White>
// Gnd                            // [G ]

#define B_FlipScreen PIN_41_D19

// Whammy Bar / POT
// +3.3v                          // [+V] <Red>
#define A_Whammy PIN_04_D01_A1    // [04] <White> - Pot Reading
// Gnd                            // [G ] <Black>

// Screen block (TWO SETS OF WIRE COLOURS - accidentally soldered the block socket wrong way around, doh)
// +3.3v                          // [+V] <Red> <Gray> - V++ (ignore pin 36)
// Gnd                            // [G ] <Black> <Brown> - set as Gnd (ignore pin 39)
// -- Spare                       // [  ] <Yellow> <Blue> - SPARE
#define I2C_SDA PIN_39_D21        // [39] <Orange> <Green> - Screen - SDA
#define I2C_SCL PIN_40_D20        // [40] <Green> <Orange> - Screen - SCK
// 41                             // [41] <Yellow> - Rotator Switch 1
// 42                             // [42] <Black> - Rotator Switch 2
// -- Spare                       // [  ] <Gray> <Red>

// LED's (Note internal using ??)
#define DATA_PIN PIN_38_D22               // NEOPIXEL

// IMPORTANT
// Assumes the first 5 inputs defined are G/R/Y/B/O buttons, and references in startup for bluetooth overrides
static Input DigitalInputs[] = {
  // { Pin, Text description, Bluetooth Input (or 0 for none), Initial State (HIGH, LOW, -1),
  // Bluetooth Pressed Operation, Bluetooth Release Operation, BluetoothSetOperation, Render operation,
  // x pos, y pos, width, height, Icon for if true, Icon for if false }
  { B_Green, "Green", BUTTON_1, LOW, &BleGamepad::press, &BleGamepad::release, NoBTSetOperation, RenderInput_Rectangle, 84, 30, 4, 5, 0, 0 },
  { B_Red, "Red", BUTTON_2, LOW, &BleGamepad::press, &BleGamepad::release, NoBTSetOperation, RenderInput_Rectangle, 77, 30, 4, 5, 0, 0 },
  { B_Yellow, "Yellow", BUTTON_4, LOW, &BleGamepad::press, &BleGamepad::release, NoBTSetOperation, RenderInput_Rectangle, 70, 30, 4, 5, 0, 0 },
   { B_Blue, "Blue", BUTTON_3, LOW, &BleGamepad::press, &BleGamepad::release, NoBTSetOperation, RenderInput_Rectangle, 63, 30, 4, 5, 0, 0 },
   { B_Orange, "Orange", BUTTON_5, LOW, &BleGamepad::press, &BleGamepad::release, NoBTSetOperation, RenderInput_Rectangle, 56, 30, 4, 5, 0, 0 },
   { B_Start, "Start", BUTTON_7, LOW, &BleGamepad::press, &BleGamepad::release, NoBTSetOperation, RenderInput_Icon, 64, 20, 16, 5, Icon_Start, 0 },
   { B_Select, "Select", BUTTON_8, LOW, &BleGamepad::press, &BleGamepad::release, NoBTSetOperation, RenderInput_DoubleIcon, 63, 40, 19, 5, Icon_Select1, 0 },
   { B_Tilt, "Tilt", BUTTON_9, LOW, &BleGamepad::press, &BleGamepad::release, NoBTSetOperation, RenderInput_Icon, 86, 40, 13, 14, Icon_Tilt, 0 }

   { B_FlipScreen, "Flip Screen", 0, -2, NoBTPressOperation, NoBTReleaseOperation, NoBTSetOperation, FlipScreen, 0, 0, 0, 0, 0, 0 }
  
   //{ PIN_07_D04_A4, "Vol+", VOLUME_INC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolUp, 0 },
  //{ PIN_05_D02_A2, "Vol-", VOLUME_DEC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolDown, 0 },
  //{ PIN_06_D03_A3, "Mute", VOLUME_MUTE_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolMute, 0 },
  //{ PIN_D12, "Menu", MENU_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
  //{ PIN_D13, "Home", HOME_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
  //{ PIN_A5, "Back", BACK_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 }
};

//#define FlipScreen_Toggle 1

int DigitalInputs_Count = sizeof(DigitalInputs) / sizeof(DigitalInputs[0]);

#define WhammyX 117
#define WhammyY 17
#define WhammyW 7
#define WhammyH 31

#define Enable_Slider1 1

static Input AnalogInputs[] = {
  { A_Whammy, "Whammy", 0, -1, 0, 0, &BleGamepad::setSlider1, RenderInput_AnalogBar_Vert, WhammyX + 2, WhammyY + 2, WhammyW - 4, WhammyH - 4, 0, 0 }
};

int AnalogInputs_Count = sizeof(AnalogInputs) / sizeof(AnalogInputs[0]);

// Hat inputs...
// Assumes hats are used Hat1 -> 4

// Hat states, for all possible hats
// (all hats are passed to bleGamepad library, even if not used)
// To define a hat without a pin/control point, set pin to 0 - e.g. below has no Up and Down
// { { 0, Hat1_Right, 0, Hat1_Left }, "1", 0, 0, RenderInput_Hat, 98, 16, 15, 15, Icon_DPad_Neutral }

signed char HatValues[] = { 0, 0, 0, 0 };

static HatInput HatInputs[] = {
  // { Up, Right, Down, Left }
  { { 0, Hat1_Right, 0, Hat1_Left }, "1", 0, 0, RenderInput_Hat, 98, 16, 15, 15, Icon_DPad_Neutral }
};

int HatInputs_Count = sizeof(HatInputs) / sizeof(HatInputs[0]);

// For LED shinanigans
Input* DigitalInputs_Green = &DigitalInputs[0];
Input* DigitalInputs_Red = &DigitalInputs[1];
Input* DigitalInputs_Yellow = &DigitalInputs[2];
Input* DigitalInputs_Blue = &DigitalInputs[3];
Input* DigitalInputs_Orange = &DigitalInputs[4];
Input* AnalogInputs_Whammy = &AnalogInputs[0];