#pragma once

#include "Structs.h"
#include "Stats.h"

// Notes

// To specify a setting specifying a function call, use NONE. If you ignore the setting, it should default to NONE. Equivalent of nullptr;
// e.g. .BluetoothSetOperation = NON

// Recommended max number of physical LED's
// Using a typical 5V USB without high-current capabilities, it should be safe to power
// 10-15 NeoPixels at moderate usage (around 20-30mA per LED)
// 5-8 NeoPixels at full brightness (60mA per LED)
// ...accounting for an extra 100 or so mA for controller
// and remembering standard USB 2.0 ports are typically rated for 500mA, and USB 3.0 ports 900 mA

// General configuration - reminder some config options are in Config.h
//#define LIVE_BATTERY                // Enable for device normally, but when testing on breadboard you might not have relevant battery or monitoring in place, triggering low battery handling. Disable to ignore these low battery checks.
#define USE_ONBOARD_LED               // Enable onboard Neopixel LED
#define STATUS_LED_COMBINE_INPUTS     // Status LED includes a generalised colour made up of Status colour + other LED's (in an approximately additive way)
#define USE_EXTERNAL_LED              // Enable external LEDs - may want to check the ExternalLED_FastLEDCount below too
#define WIFI                          // Enable WiFi (required for web server)
#define WEBSERVER                     // Enable on board web server

#define CLEAR_STATS_ON_FLIP             // Resets stats counter when screen flipped (just a handy way for a manual zeroing without needing an extra button)

// Go into idle LED effects mode after 10 seconds
#define IDLE_TIMEOUT 10.0

// =====
// LED's

// Mappings to physical Neopixel/equivalent LED offsets
#define LED_Status          0
// #define LED_Green           1
// #define LED_Red             2
// #define LED_Yellow          3
// #define LED_Blue            4
// #define LED_Orange          5
// #define LED_Tilt            6
// #define LED_Status_Copy     7
// #define LED_Hat_Off         8
// #define LED_Hat_1           9
// #define LED_Hat_2           10
// #define LED_Hat_3           11
// #define LED_Hat_4           12
// #define LED_Hat_5           13
// #define LED_Hat_6           14
// #define LED_Hat_7           15
// #define LED_Hat_8           16
// #define LED_DigitalTest     17
// #define LED_DigitalTest_Count (15+17)
// // Non fading LED's after this point
// #define LED_Whammy          (LED_DigitalTest + LED_DigitalTest_Count)
// #define LED_Whammy_Count    8

#define LED_TOTALCOUNT      1

#define LED_BRIGHTNESS     20                       // 0->255 - note FastLED has 1 global brightness setting, so affects both onboard and external LED's

#define ONBOARD_LED_FADE_RATE (1.0 / 0.2)           // 0.15 is the total amount of seconds a complete 255->0 fade will be over

#define EXTERNAL_LED_FADE_RATE (1.0 / 0.1)          // 0.075 is the total amount of seconds a complete 255->0 fade will be over (actual result is approximate)
#define EXTERNAL_LED_TYPE WS2812B                   // WS2852
#define EXTERNAL_LED_COLOR_ORDER GRB

// External pin EXTERNAL_LED_PIN defined lower down
#define ExternalLED_Count LED_TOTALCOUNT            // Space for all LEDs - in our case 5 neck buttons, 1 status LED + 1 status LED clone
#define ExternalLED_FadeCount LED_Status            // Auto-fade of LED's - basically fades all LED's in ExternalLED array up to this point.
                                                    // RECOMMENDATION: if physically possible, stick all fading LED's at the start of your array, and non fading ones at the end - less overhead then
                                                    // Less overhead then                                          
#define ExternalLED_FastLEDCount LED_TOTALCOUNT     // Match ExternalLED_Count for all LEDs, but set to e.g. 1 (assuming LED 0 is the status led) for only the first status LED to be used
                                                    // Lets us simplify code complexity for the sake of processing some extra LED logic but only want the status one installed (save battery life mode!)
#define ExternalLED_StatusLED LED_Status            // LED to use as an external Status - copy of internal status. Do not define if you have no external status led.

// List of LED's we want cloning (lets you copy LED values between each other)
// e.g. when you might have multiple physical LED's that you want to share the same value, such as a light ring where you want the whole thing lit up at multiple points
// Actual definition in .cpp file
extern IntPair LEDClones[];
extern int LEDClones_Count;

// ======================
// User Interface Related
#define uiGuitar_xPos 12
#define uiGuitar_yPos 16

#define uiWhammyX 117
#define uiWhammyY 17
#define uiWhammyW 7
#define uiWhammyH 31

extern IconRun ControllerGfx[];

// ===============
// PIN definitions
// pud = pull up pull down resistor available
// Do not use ADC2 pins for ADC - ESP32-S3 ADC2 has issues (especially if using wireless/bluetooth) and is easier just to not use it than faff around

// Extra details on configuration and what it means - see project Wiki https://github.com/ITheP/GamePad/wiki/ESP32-S3

// SIDE ONE
//      +3v3
//      +3v3
//      RST
#define PIN_04_D01_A1 4   // [Whammy] OK pud ADC1_3 Touch_01
#define PIN_05_D02_A2 5   // [Hat1 R] OK pud ADC1_4 Touch_02
#define PIN_06_D03_A3 6   // [Hat1 L] OK pud ADC1_5 Touch_03
#define PIN_07_D04_A4 7   // [Blue  ] OK pud ADC1_6 Touch_04
#define PIN_15_D05    15  // [Green ] OK pud adc2_4           - Do not use for ADC
#define PIN_16_D06    16  // [Orange] OK pud adc2_5           - Do not use for ADC
#define PIN_17_D07    17  // [Yellow] OK pud adc2_6           - Do not use for ADC
#define PIN_18_D08    18  // [      ] OK pud adc2_7           - Do not use for ADC
#define PIN_08_D09    8   // [Red   ] OK pud ADC1_7 Touch_08  - I2C SDA default
//      PIN_3         XX  //             pud ADC1_2 Touch_03  - Boot Strapping pin (JTAG signal source) - DO NOT USE
//      PIN_46        XX  //                                  - Boot Strapping pin (Chip boot mode and ROM messages printing), input only, no internal pull up/down - DO NOT USE
#define PIN_09_D10_A5 9   // [      ] OK pud ADC1_8 Touch_09  - I2C SCL Default
#define PIN_10_D11_A6 10  // [Tilt  ] OK pud ACD1_9 Touch_10  -                      SPI3 CS
#define PIN_11_D12    11  // [      ] OK pud adc2_0 Touch_11  - Do not use for ADC - SPI3 MOSI
#define PIN_12_D13    12  // [      ] OK pud adc2_1 Touch_12  - Do not use for ADC - SPI3 CLK
#define PIN_13_D14    13  // [Start ] OK pud adc2_2 Touch_13  - Do not use for ADC - SPI3 MISO
#define PIN_14_D15    14  // [Select] OK pud adc2_3 Touch_14  - Do not use for ADC
//      +5v in            //                                  - +5v from USB if IN-OUT jumper bridged
//      Gnd

// SIDE TWO
//      Gnd
//      TX            43  //                                  - UART0 TX/Debug
//      RX            44  //                                  - UART0 RX/Debug
#define PIN_01_D16_A7 1   // [      ] OK pud ADC1_0 Touch_01
#define PIN_02_D17_A8 2   // [      ] OK pud ADC1_1 Touch_02
#define PIN_42_D18    42  // [      ] ok                      - JTAG MTMS
#define PIN_41_D19    41  // [FlipSc] ok                      - JTAG MTDI
#define PIN_40_D20    40  // [Screen] ok                      - JTAG MTDO
#define PIN_39_D21    39  // [Screen] ok                      - JTAG MTCK, SPI2 CS
#define PIN_38_D22    38  // [ExtLED] ok                      - External Status LED
//      PIN_37            // [      ] ok                      - SPI2 MISO - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
//      PIN_36            // [      ] ok                      - SPI2 CLK  - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
//      PIN_35            // [      ] ok                      - SPI2 MOSI - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
//      PIN_0             //                                  - Boot Strapping Pin Boot Mode
//      PIN_45            //                                  - Boot Strapping Pin VDD SPI Voltage (VDD_SPI voltage, selects between 1.8v and 3.3v)
#define PIN_48_D23    48  // [IntLED] ok                      - Internal NEOPIXEL (default internal pin reference)
#define PIN_47_D24    47  // [ExtLED] ok                      - External LED's
#define PIN_21_D25    21  // [      ] OK pud
//      PIN_20        XX  //                                  - USB_D+ - if reconfigured as normal GPIO, USB-JTAG functionality unavailable - i.e. don't expect USB to work! - DO NOT USE
//      PIN 19        XX  //                                  - USB_D- - DO NOT USE
//      Gnd
//      Gnd

// ==================================================
// All possible points ordered into Blocks/connectors
// Documented wire color in <brackets> was used in prototype

// Battery monitor ... 10k -> +3.3, 20k -> Gnd
#define BATTERY_MONITOR_PIN     PIN_02_D17_A8 // Battery Voltage - 20K ohm to Gnd + 10K ohm to +ve

// Guitar Neck Buttons
#define HAT1_Up_PIN             PIN_05_D02_A2 // [05] <Gray>
#define HAT1_Down_PIN           PIN_06_D03_A3 // [06] <Brown>
#define BUTTON_Blue_PIN         PIN_07_D04_A4 // [07] <Blue> 4th
#define BUTTON_Green_PIN        PIN_15_D05    // [15] <Green> 1st
#define BUTTON_Orange_PIN       PIN_16_D06    // [16] <Orange> 5th
#define BUTTON_Yellow_PIN       PIN_17_D07    // [17] <Yellow> 3rd
// Gnd                                        // [G ] <Black> - Set as Gnd
#define BUTTON_Red_PIN          PIN_08_D09    // [18] <Red> 2nd

// Extra buttons
#define BUTTON_Tilt_PIN         PIN_10_D11_A6 // [10] < > Tilt Sensor
// 11 Spare?                                  // [11] <White>
// 12 Spare?                                  // [12] <Black>

// End Buttons
#define BUTTON_Start_PIN        PIN_13_D14    // [13] <Red>
#define BUTTON_Select_PIN       PIN_12_D13    // PIN_14_D15    // [14] <White>
// Gnd                                        // [G ]

#define BUTTON_FlipScreen_PIN   PIN_41_D19

// Whammy Bar / POT
// +3.3v                                      // [+V] <Red>
#define ANALOG_Whammy_PIN       PIN_04_D01_A1 // [04] <White> - Pot Reading
// Gnd                                        // [G ] <Black>

// Screen block (TWO SETS OF WIRE COLOURS - accidentally soldered the block socket wrong way around, doh, so lists what colour should have been, then what was actually used
// +3.3v                                      // [+V] <Red> <Gray> - V++ (ignore pin 36)
// Gnd                                        // [G ] <Black> <Brown> - set as Gnd (ignore pin 39)
#define EXTERNAL_LED_PIN        PIN_38_D22    // [  ] <Yellow> <Blue> - External NeoPixel Status LED (was PIN_47_D24)
#define I2C_SDA                 PIN_39_D21    // [39] <Orange> <Green> - Screen - SDA
#define I2C_SCL                 PIN_40_D20    // [40] <Green> <Orange> - Screen - SCK
// 41                                         // [41] <Yellow> - Rotator Switch 1
// 42                                         // [42] <Black> - Rotator Switch 2
// -- Spare                                   // [  ] <Gray> <Red>

// Onboard pins
#define ONBOARD_LED_PIN         PIN_48_D23

// Inputs defined individually to make referencing them multiple times easier elsewhere (if required)

// ========================
// Digital Inputs (buttons)

// Input DigitalInput_Green;
// Input DigitalInput_Red;
// Input DigitalInput_Yellow;
// Input DigitalInput_Blue;
// Input DigitalInput_Orange;
// Input DigitalInput_Start;
// Input DigitalInput_Select;
// Input DigitalInput_Tilt;

#define ENABLE_FLIP_SCREEN // Required if below is defined
//#define FLIP_SCREEN_TOGGLE 1 // FlipScreen can either toggle on and off with a button press (enable), or holding a button down sets its flipped state (disable)
extern Input DigitalInput_FlipScreen;

// For controlling configuration menu
extern uint8_t BootPin_StartInConfiguration;
extern uint8_t Menu_UpPin;
extern char Menu_UpLabel[];
extern uint8_t Menu_DownPin;
extern char Menu_DownLabel[];
extern uint8_t MenuSelectPin;
extern char Menu_SelectLabel[];
extern uint8_t Menu_BackPin;
extern char Menu_BackLabel[];

// Buttons used for configuration menu
extern Input *DigitalInputs_ConfigMenu[];
extern Input DigitalInput_Config_Up;
extern Input DigitalInput_Config_Down;
extern Input DigitalInput_Config_Select;
extern Input DigitalInput_Config_Back;


// DigitalInput array, collated list of all digital inputs (buttons) iterated over to check current state of each input
extern Input *DigitalInputs[];

// =============
// Analog inputs

#define Enable_Slider1 1

// Specific inputs we need references to
//Input AnalogInputs_Whammy;

extern Input *AnalogInputs[];

// ==========
// Hat inputs

// Assumes hats are used Hat1 -> 4

// Hat states, for all possible hats
// (for simplicity, all hats are passed to bleGamepad library, even if not used)
// To define a hat without a pin/control point, set pin to NONE - e.g. might only want to use up/down but not left/right

extern unsigned char HatValues[];

// Hat used for up/down strum bar
//HatInput Hat0;

extern HatInput *HatInputs[];

// -----------------------------------------------------
// Array sizes

extern IntPair LEDClones[];
extern int LEDClones_Count;

extern Stats Stats_Neck;
extern Stats Stats_Green;
extern Stats Stats_Red;
extern Stats Stats_Yellow;
extern Stats Stats_Blue;
extern Stats Stats_Orange;

extern Stats Stats_StrumBar;
extern Stats Stats_HatUp;
extern Stats Stats_HatDown;

extern Stats *AllStats[];
extern int AllStats_Count;

extern int ControllerGfx_RunCount;
extern int AnalogInputs_Count;
extern int HatInputs_Count;
extern int DigitalInputs_Count;

extern ExternalLEDConfig *MiscLEDEffects[];
extern ExternalLEDConfig *IdleLEDEffects[];

extern int ControllerGfx_RunCount;
extern int AnalogInputs_Count;
extern int HatInputs_Count;
extern int DigitalInputs_Count;
extern int DigitalInputs_ConfigMenu_Count;
extern int MiscLEDEffects_Count;
extern int IdleLEDEffects_Count;

// -----------------------------------------------------
// Special case code specific to this controller

void Custom_RenderHatStrumState(HatInput *hatInput);

// -----------------------------------------------------
// Version references
extern char ControllerDeviceNameType[];
extern char ControllerType[];
extern char ModelNumber[];
extern char FirmwareRevision[];
extern char HardwareRevision[];
extern char SoftwareRevision[];

static const char *ConfigHelpText[] = {
    "Hold Green + strum",
    "up/down to scroll this",
    "help text.",
    "",
    "Strum up/down to change menu.",
    "Green to go into an option,",
    "Red, to back out again.",
    "",
    "Once changes are complete,",
    "don't forget to go to the save",
    "menu option to save your",
    "changes.",
    "",
    "Profile",
    "Select a profile 0-5",
    "Profile 0 is default",
    "1-5 are accessed on",
    "device startup by holding",
    "down corresponding green,",
    "red, yellow, blue or orange",
    "buttons on the guitar neck.",
    "This lets you e.g. have a",
    "separate controller profile",
    "for different devices.",
    "You can copy profile settings",
    "to make it easy to duplicate",
    "WiFi settings etc.",
    "Note that controller will",
    "automatically have different",
    "bluetooth device name per",
    "profile on boot, and will",
    "need pairing for each profile",
    "",
    "WiFi Set-up",
    "- Check current status",
    "- Select access point",
    "- Set username",
    "- Set password",
    "Note that username and",
    "password are saved in flash",
    "memory in an encrypted format.",
    ""
};