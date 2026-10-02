#pragma once

#include "Structs.h"
#include "Stats.h"
#include "RenderText.h"
#include "IconMappings.h"
#include "ConfigManager.h"
#include <string>
#include <map>
#include <optional>
#include <cstring>

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
#define LIVE_BATTERY              // Enable for device normally, but when testing on breadboard you might not have relevant battery or monitoring in place, triggering low battery handling. Disable to ignore these low battery checks.
#define USE_ONBOARD_LED           // Enable onboard Neopixel LED
#define STATUS_LED_COMBINE_INPUTS // Status LED includes a generalised colour made up of Status colour + other LED's (in an approximately additive way)
#define USE_EXTERNAL_LED          // Enable external LEDs - may want to check the ExternalLED_FastLEDCount below too
#define WIFI                      // Enable WiFi (required for web server)
#define WEBSERVER                 // Enable on board web server
#define REQUIRE_DISPLAY           // No screen - no run! Ignore to bypass screen safety checks. Code wise screen may still be configured and used but physically not be there to show anything.

#define CLEAR_STATS_ON_FLIP // Resets stats counter when screen flipped (just a handy way for a manual zeroing without needing an extra button)

// Idle effect default timings and settings
#define DEFAULT_IDLE_LED_TIMEOUT 10.0     // Seconds before LED's go into Idle mode
#define DEFAULT_IDLE_SCREEN_TIMEOUT 30.0  // Seconds before Screen goes into Idle mode
#define DEFAULT_IDLE_EFFECT_RESTART 60.0  // Seconds before screen restarts it's idle effect (keeps it more interesting)
#define DEFAULT_IDLE_LED_RUN_EXCLUSIVELY  // Idle effect LED's run on their own - all other LED effects stop.
                                  // You may want both to be processed at the same time (especially if the Idle effects are only set to process on a subset of LED's)

// =====
// LED's

// Mappings to physical Neopixel/equivalent LED offsets
enum class LEDStrip {
    Status,
    //Tilt,
    Green,
    Red,
    Yellow,
    Blue,
    Orange,
    Orange_Neck,
    Blue_Neck,
    Yellow_Neck,
    Red_Neck,
    Green_Neck,

    COUNT   // Should auto-populate as part of the enum as last item
};

#define DEFAULT_LED_BRIGHTNESS 200 // 0->255 - note FastLED has 1 global brightness setting, so affects both onboard and external LED's

#define ONBOARD_LED_FADE_RATE (1.0 / 0.2) // 0.15 is the total amount of seconds a complete 255->0 fade will be over

#define EXTERNAL_LED_FADE_RATE (1.0 / 0.1) // 0.075 is the total amount of seconds a complete 255->0 fade will be over (actual result is approximate)
#define EXTERNAL_LED_TYPE WS2812B          // WS2852
#define EXTERNAL_LED_COLOR_ORDER GRB

// External pin EXTERNAL_LED_PIN defined lower down
#define ExternalLED_Count (int)LEDStrip::COUNT //LED_TOTALCOUNT        // Space for all LEDs - in our case 5 neck buttons, 1 status LED + 1 status LED clone
#define ExternalLED_FadeCount ((int)LEDStrip::Orange+1)  // LED_Status        // Auto-fade of LED's - basically fades all LED's in ExternalLED array up to this point.
                                                // RECOMMENDATION: if physically possible, stick all fading LED's at the start of your array, and non fading ones at the end - less overhead then
                                                // Less overhead then
#define ExternalLED_FastLEDCount (int)LEDStrip::COUNT // LED_TOTALCOUNT // Match ExternalLED_Count for all LEDs, but set to e.g. 1 (assuming LED 0 is the status led) for only the first status LED to be used
                                                // Lets us simplify code complexity for the sake of processing some extra LED logic but only want the status one installed (save battery life mode!)
#define ExternalLED_StatusLED (int)LEDStrip::Status  // LED_Status        // LED to use as an external Status - copy of internal status. Do not define if you have no external status led.

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
// Do not use ADC2 pins for ADC - ESP32-S3 ADC2 has issues, especially if using wireless/bluetooth. It shares hardware with the Wi‑Fi block,
// so reads can fail, block, or get noisy while Wi‑Fi is active). Is easier just to not use it than faff around

// Extra details on configuration and what it means - see project Wiki https://github.com/ITheP/GamePad/wiki/ESP32-S3

// SIDE ONE
//      Pin Name       Pin Number  [ Device Usage   ] Usable?                 - Extra Notes + Default usage
//      +3v3
//      +3v3
//      RST
#define PIN_04_D01_A1   4       // [Orange          ] ok pud ADC1_3 Touch_01
#define PIN_05_D02_A2   5       // [Power Sense     ] ok pud ADC1_4 Touch_02  - Hardware locked power sensing pin (Olimex ESP32-S3-DevKit-Lipo)
#define PIN_06_D03_A3   6       // [Battery Sense   ] ok pud ADC1_5 Touch_03  - Hardware locked battery sensing pin (Olimex ESP32-S3-DevKit-Lipo)
#define PIN_07_D04_A4   7       // [Red             ] ok pud ADC1_6 Touch_04
#define PIN_15_D05      15      // [                ] ok pud adc2_4           - Do not use for ADC
#define PIN_16_D06      16      // [                ] ok pud adc2_5           - Do not use for ADC
#define PIN_17_D07      17      // [Hat1 Left       ] ok pud adc2_6           - Do not use for ADC
#define PIN_18_D08      18      // [Hat1 Right      ] OK pud adc2_7           - Do not use for ADC
#define PIN_08_D09_A5   8       // [Capacitor Strip ] ok pud ADC1_7 Touch_08  - I2C SDA default
//      PIN_3           XX                               pud ADC1_2 Touch_03  - DO NOT USE - Boot Strapping pin (JTAG signal source)
//      PIN_46          XX                                                    - DO NOT USE - Boot Strapping pin (Chip boot mode and ROM messages printing), input only, no internal pull up/down
#define PIN_09_D10_A6   9       // [Green           ] ok pud ADC1_8 Touch_09  - I2C SCL Default
#define PIN_10_D11_A7   10      // [Yellow          ] OK pud ADC1_9 Touch_10  -                      H/W SPI3 CS
#define PIN_11_D12      11      // [Hat1 Up         ] ok pud adc2_0 Touch_11  - Do not use for ADC - H/W SPI3 MOSI/SDA
#define PIN_12_D13      12      // [Hat1 Down       ] ok pud adc2_1 Touch_12  - Do not use for ADC - H/W SPI3 SCK/CLK
#define PIN_13_D14      13      // [Start           ] ok pud adc2_2 Touch_13  - Do not use for ADC - H/W SPI3 MISO
#define PIN_14_D15      14      // [Select          ] ok pud adc2_3 Touch_14  - Do not use for ADC
//      +5v in                  //                                            - +5v from USB if IN-OUT jumper bridged
//      Gnd

// SIDE TWO
//      Pin Name       Pin Number  [Our Device Usage] Usable?                 - Extra Notes
//      Gnd
//      TX              43      //                                            - UART0 TX/Debug
//      RX              44      //                                            - UART0 RX/Debug
#define PIN_01_D16_A8   1       // [Whammy          ] OK pud ADC1_0 Touch_01
#define PIN_02_D17_A9   2       // [Blue            ] OK pud ADC1_1 Touch_02
#define PIN_42_D18      42      // [External LED    ] OK                      - JTAG MTMS
#define PIN_41_D19      41      // [                ] ok                      - JTAG MTDI
#define PIN_40_D20      40      // [Screen SPI2 DC  ] ok                      - JTAG MTDO
#define PIN_39_D21      39      // [Screen SPI2 CS  ] ok                      - JTAG MTCK, SPI2 CS
#define PIN_38_D22      38      // [Screen SPI2 CLK ] ok                      - H/W SPI2 SCK/CLK (default, cleanest clock)
#define PIN_37_D23      37      // [                ] OK                      - H/W SPI2 MISO                   - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
#define PIN_36_D24      36      // [Screen SPI2 RST ] ok                      - H/W SPI2 SCK/CLK (alternate)    - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
#define PIN_35_D25      35      // [Screen SPI2 MOSI] ok                      - H/W SPI2 MOSI/SDA               - Ok to use if not used for Octal SPI Flash or PSRAM (model specific)
//      PIN_0                   //                                            - Boot Strapping Pin Boot Mode
//      PIN_45                  //                                            - Boot Strapping Pin VDD SPI Voltage (VDD_SPI voltage, selects between 1.8v and 3.3v)
#define PIN_48_D23      48      // [Int. Status LED ] ok                      - Internal NEOPIXEL (default internal pin reference)
#define PIN_47_D24      47      // [Tilt            ] ok                      - External LEDs (1st is clone of onboard status LED)
#define PIN_21_D25      21      // [FlipScreen      ] OK pud                  -
//      PIN_20          XX                                                    - USB_D+ - DO NOT USE - If reconfigured as normal GPIO, USB-JTAG functionality unavailable - i.e. don't expect USB to work!
//      PIN 19          XX                                                    - USB_D- - DO NOT USE
//      Gnd
//      Gnd

// ==================================================
// All possible points ordered into Blocks/connectors
// Documented wire color in <brackets> was used in prototype

// Battery monitor ... 10k -> +3.3, 20k -> Gnd
// Following is based on the Olimex ESP32-S3-DevKit-Lipo with built in battery and power monitoring
#define POWER_MONITOR_PIN       PIN_05_D02_A2   // Power Voltage
#define BATTERY_MONITOR_PIN     PIN_06_D03_A3   // Battery Voltage - Olimax voltage divider has 470k ohm to +ve and to gnd

//#define ADC_RESOLUTION 4095.0
//#define ADC_REF        2.2         // 3.3
// Correct ratios from schematic
// 4.1333
// Didn't give accurate results!
// From manual readings
// 3.536
// Ended up ignoring ratio's as it didn't really matter for our purposes, don't need super accuracy
//#define BAT_DIVIDER_RATIO 3.536
//#define PWR_DIVIDER_RATIO  5.6808
// 5V through divider gives ~880mV at ADC
// Anything above 0.4v means external power present
//#define PWR_PRESENT_THRESHOLD 0.4

// // Min/Max raw readings from battery monitoring pin
// #define BATTERY_MINV 3.3
// #define BATTERY_MIN 1731.0

// #define BATTERY_MAXV 4.0
// #define BATTERY_MAX 2090.0

// #define BATTERY_OVERCHARGEV 4.2
// #define BATTERY_OVERCHARGE 2193.0

// // Min/Max raw readings from battery monitoring pin WHEN USB
// // connected and charging (higher voltage than actual battery voltage)
// // Final %'s based on below can be translated into an equivalent
// // theoretical battery voltage
// #define BATTERY_CHARGING_MINV 3.37
// #define BATTERY_CHARGING_MIN 1598.0

// #define BATTERY_CHARGING_MAXV 4.00
// #define BATTERY_CHARGING_MAX 1977.0

// #define BATTERY_CHARGING_OVERCHARGEV 4.20
// #define BATTERY_CHARGING_OVERCHARGE 2097.0

// // Power pin with no battery connected
// #define POWER_PLUS_CHARGING 512.0
// #define POWER_ONLY 4095.0

// // Power monitoring pin, anything above this we assume power is supplied (realistically reads 0 for no power and 3980-4096 when power is there)
// #define PWR_PRESENT_THRESHOLD 1024

// Empty battery equivalent - 0% full (can be less, but then considered undercharged)
#define BATTERY_MIN 1731
#define BATTERY_MIN_V 3.3
// Full battery equivalent - 100% full (approx 80% physically charged, can be more, but then considered overcharged)
#define BATTERY_FULL 2090 // 2100 is when battery is ~4.015// 2090
#define BATTERY_FULL_V 4.0
// Theoretical actual battery max - ~130% overcharged (physically fine, shortens life of battery)
#define BATTERY_MAX 2500 // MADE UP 2193
#define BATTERY_MAX_V 4.20

// Magic number where when power pin reads 4095 we can tell if the battery is connected or not. < this number = no battery, just power
#define BATTERY_CHECK 2235 // 1920

// Readings when power plugged in and on battery
// 0%->100% results here map to 0%->100% equivalent of battery range
// so theoretically when plugging in and out a USB cable, code changes range it calculates the % battery charge as an equivalent to as if it was just the battery
#define CHARGING_MIN 1598
#define CHARGING_MIN_V 3.37
// Measuring 2280 @ 4.053v
#define CHARGING_FULL 2250 // measuring 4.045v // 1977
#define CHARGING_FULL_V 4.0
#define CHARGING_MAX 2500 // MADE UP 2097
#define CHARGING_MAX_V 4.20

// Consider power to be present if power pin above this (allows for some noise on power pin when not powered)
#define POWER_PRESENT 512
#define POWER_MAX 4095

// Guitar Neck Buttons [Red 11 block]
// +3.3v                                        // [+v] +3.3v LED Power <Red Wire>
// LED Through from data path
// Gnd                                          // [G ] Gnd
#define BUTTON_Orange_PIN       PIN_04_D01_A1   // [04] Orange
#define BUTTON_Blue_PIN         PIN_02_D17_A9   // [02] Blue
#define BUTTON_Yellow_PIN       PIN_10_D11_A7   // [10] Yellow
#define BUTTON_Red_PIN          PIN_07_D04_A4   // [07] Red
#define BUTTON_Green_PIN        PIN_09_D10_A6   // [09] Green
// +3.3v                                        // [+v] +3.3v Capacitance Strip + Hall Sensor Power
#define ANALOG_Capacitor_PIN    PIN_08_D09_A5   // [08] Button simulator Capacitance Strip
// Gnd                                          // [G ] Gnd <Red Wire>

// Direction/Hat buttons [Yellow 7 block]
#define HAT1_Right_PIN          PIN_18_D08      // [18] <Red Wire>
// Gnd                                          // [G ] Gnd
#define HAT1_Left_PIN           PIN_17_D07      // [17]
// Gnd                                          // [G ] Gnd
#define HAT1_Up_PIN             PIN_11_D12      // [11] Goes to both Hat Up and Up Switch
// Gnd                                          // [G ] Gnd
#define HAT1_Down_PIN           PIN_12_D13      // [12]Goes to both Hat Down and Down Switch

// Start Select [Black 4 block]
#define BUTTON_Start_PIN        PIN_13_D14      // [13]
// Gnd                                          // [G ]
#define BUTTON_Select_PIN       PIN_14_D15      // [14]
// Gnd                                          // [G ] <Black Wire>

// Whammy Bar / POT [Red 3 block]
// Gnd                                          // [G ]
#define ANALOG_Whammy_PIN       PIN_01_D16_A8   // [01]
// +3.3v
                                      // [+V]
// External LEDs (start point) [Green 3 block]
// +3.3v                                        // [+v] +3.3v
#define EXTERNAL_LED_PIN        PIN_42_D18      // [  ] External NeoPixel Status LED
// Gnd                                          // [G ] Gnd <Black Wire>
   
// Screen block [Blue 8 block]
// +3.3v                                        // [+V] +3.3v
// Gnd                                          // [G ] Gnd
#define SCREEN_SPI2_DC          PIN_40_D20      // [40] Screen - DC
#define SCREEN_SPI2_CS          PIN_39_D21      // [39] Screen - CS
#define SCREEN_SPI2_SCK_PIN     PIN_38_D22      // [38] Screen SPI2 SCK/CLK
#define SCREEN_SPI2_MISO_PIN    PIN_37_D23      // [37] Screen SPI2 MISO (not used in this case)
#define SCREEN_SPI2_RST_PIN     PIN_36_D24      // [36] Screen RST
#define SCREEN_SPI2_MOSI_PIN    PIN_35_D25      // [35] Screen SPI2 MOSI/SDA

// Tilt [Blue 2 block]
#define BUTTON_Tilt_PIN         PIN_47_D24      // [47] Tilt Sensor
// Gnd

// Flip Screen [Green 2 block]
#define BUTTON_FlipScreen_PIN   PIN_21_D25      // [21] Flip Screen
// Gnd
                                       // [G ]


// Onboard pins
#define ONBOARD_LED_PIN         PIN_48_D23      // [48] Not actually exposed as pin on h/w

// Inputs defined individually to make referencing them multiple times easier elsewhere (if required)

// ========================
// Digital Inputs (buttons)

// extern Input DigitalInput_Green;
// extern Input DigitalInput_Red;
// extern Input DigitalInput_Yellow;
// extern Input DigitalInput_Blue;
// extern Input DigitalInput_Orange;
// extern Input DigitalInput_Start;
// extern Input DigitalInput_Select;
// extern Input DigitalInput_Tilt;

#define ENABLE_FLIP_SCREEN          // Required if below is defined
#define FLIP_SCREEN_TOGGLE 1 // FlipScreen can either toggle on and off with a button press (enable), or holding a button down sets its flipped state (disable)
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

// Buttons used for battery boot
#define BATTERY_HASINPUTS 1
extern Input *DigitalInputs_Battery[];
extern Input DigitalInput_Battery_Continue;
extern Input DigitalInput_Battery_ExtraInfo;
extern Input *AnalogInputs_Battery[];

// Buttons used for battery boot status/charging screen
// Continue to full boot
extern Input DigitalInput_Battery_Continue;
// Extra info shown in screen
extern Input DigitalInput_Battery_ExtraInfo;

// DigitalInput array, collated list of all digital inputs (buttons) iterated over to check current state of each input
extern Input *DigitalInputs[];

// =============
// Pulse inputs

extern PulseInput *PulseInputs[];
// Bit of hardcoding required to attach pulse tracking code to interrupts in a specific way
void AttachPulseInputInterrupts();

// =============
// Analog inputs

#define Enable_Slider1 1

// Specific inputs we need references to
// extern Input AnalogInputs_Whammy;

extern Input *AnalogInputs[];

// ==========
// Hat inputs

// Assumes hats are used Hat1 -> 4

// Hat states, for all possible hats
// (for simplicity, all hats are passed to bleGamepad library, even if not used)
// To define a hat without a pin/control point, set pin to NONE - e.g. might only want to use up/down but not left/right

extern unsigned char HatValues[];

// Hat used for up/down strum bar
// HatInput Hat0;

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
extern int PulseInputs_Count;
extern int AnalogInputs_Count;
extern int HatInputs_Count;
extern int DigitalInputs_Count;
extern int DigitalInputs_ConfigMenu_Count;
extern int DigitalInputs_Battery_Count;
extern int AnalogInputs_Battery_Count;
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

// Config menu stuff

// Config menu text - injected into UI in relevant places
#define DIGITALINPUT_CONFIG_UP_LABEL "Strum Up"
#define DIGITALINPUT_CONFIG_DOWN_LABEL "Strum Down"
#define DIGITALINPUT_CONFIG_SELECT_LABEL "Green Button"
#define DIGITALINPUT_CONFIG_BACK_LABEL "Red Button"

// Battery boot up extra text
#define DIGITALINPUT_BATTERY_CONTINUE_LABEL "Start Button"
#define DIGITALINPUT_BATTERY_EXTRAINFO_LABEL "Extra Info Button"
