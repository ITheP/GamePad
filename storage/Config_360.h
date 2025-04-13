// PIN Allocation
// ESP32-S3...
// https://www.luisllamas.es/en/esp32-s3-hardware-details-pinout/
// GPIOx = general purpose I/O pin (i.e. use as button input)
// ADCy_x = Analog capable input (i.e. use as variable input)
// Tx = Capacitive touch pin
// Pins:

// Pins listed in physical order

// SIDE ONE
// +3v3
// +3v3
// RST
#define PIN_04_D01_A1 4  // Analog 1 - Whammy Bar
#define PIN_05_D02_A2 5  // Analog 2 - Volume Down
#define PIN_06_D03_A3 6  // Analog 3 - Mute
#define PIN_07_D04_A4 7  // Analog 4 - Volume Up

// MAIN GUITAR NECK CONTROLS
// 6 pin block (including gnd)

// ADC2_CH4     GPIO15 = OK
#define PIN_D1 15  // Button 1 (Green)

// ADC2_CH5     GPIO16 = OK
#define PIN_D2 16  // Button 2 (Red)

// ADC2_CH6    GPIO17 = OK
#define PIN_D3 18  // Button 4 (Yellow)

// ADC2_CH7     GPIO18 = OK
#define PIN_D4 17  // Button 3 (Blue)

// ADC1_CH7 T8  GPIO8  = OK (SDA default)
#define PIN_D5 8  // Button 5 (Orange)

// ADC1_CH2 T3  GPIO3  = Strapping pin (JTAG signal source)
//             3  // DO NOT USE

// GPIO46 = Strapping pin (Chip boot mode and ROM messages printing)
//             46 // DO NOT USE

// Controlls Board - DPAD CONTROL + START/SELECT

// ADC1_CH8 T9  GPIO9  = OK (SCL default)
#define PIN_D7 9  // Hat Right

// ADC1_CH9 T10 GPIO10 = OK (SPI3 CS)
#define PIN_D6 10  // Hat Up + Strum Up

// ADC2_CH0 T11 GPIO11 = OK (SPI3 MOSI)
#define PIN_D8 12  // Hat Down + Strum Down

// ADC2_CH1 T12 GPIO12 = OK (SPI3 CLK)
#define PIN_D9 11  // Hat Left

// ADC2_CH2 T13 GPIO13 = OK (SPI3 MISO)
#define PIN_D10 13  // Start

// ADC2_CH3 T14 GPIO14 = OK
#define PIN_D11 14  // Select

// +5v in
// Gnd

// SIDE TWO

// Gnd
// Gnd

//   GPIO0->21 (have internal pull-up/down resistors)
//              GPIO0  = Strapping pin (chip boot mode)

// ADC1_CH0 T1  GPIO1  = OK
#define PIN_BATTERYMONITOR 1  // BATTERY VOLTAGE
// 20K ohm to Gnd
// 10K ohm to +ve

// ADC1_CH1 T2  GPIO2  = OK
#define PIN_A5 2  // Guide

// ADC2_CH8     GPIO19 = USB-JTAG (USB_D+ - if reconfigured as normal GPIO, USB-JTAG functionality unavailable - i.e. don't expect USB to work!)
//             19 // DO NOT USE

// ADC2_CH0     GPIO20 = USB-JTAG (USB_D-)
//             20 // DO NOT USE

//              GPIO21 = OK

//   GPIO22->25
//   GPIO26->48 (input only with no pull-up/down resistors)
//              GPIO26->32 usually used for SPI flash and PSRAM, not recommended for other uses
//              ..
//              GPIO33->37 not recommended in ESP32-S3R8 / ESP32-S3R8V if using SPI or PSRAM (usually not using these)
//              GPIO35 = -- (SPI2 MOSI)
//              GPIO36 = -- (SPI2 CLK)
//              GPIO37 = -- (SPI2 MISO)

//              GPIO38 = OK
#define DATA_PIN 38  // NEOPIXEL

//              GPIO39 = -- (SPI2 CS)
//              ...

//              GPIO39->42 cannot be used if using JTAG debugging

//              GPIO39 = OK
#define I2C_SDA 39 // SPARE

//              GPIO40 = OK
#define I2C_SCL 40  // SPARE

//              GPIO41 = OK
#define PIN_D12 41  // Home

//              GPIO42 = OK
#define PIN_D13 42  // Menu

//              GPIO43 = -- (UOTX - not recommended)
//              GPIO44 = -- (UORX - not recommended)
//              GPIO45 = Strapping pin (VDD_SPI voltage, selects between 1.8v and 3.3v)

//              EN     = Reset

// Wiring blocks overview...
// +3.3                           [Red]
// 4    // Whammy POT             [White]
// Gnd                            [Black]

// NOTE some missoldering on specific controlers means some pin correction required :)

// Guitar Neck Buttons
// 15   // 1 - Green
// 16   // 2 - Red
// 17   // 3 - Blue 18
// 18   // 4 - Yellow 17
// 8    // 5 - Orange
// Gnd

// Controls
// 9    // Hat Up + Strum Up      [Gray] 10 
// 10   // Hat Right              [Brown] 9
// 11   // Hat Down + Strum Down  [Blue] 12
// 12   // Hat Left               [Green] 11
// 13   // Start                  [Orange]
// 14   // Select                 [Yellow]
// 2    // Guide (long wire)      [Black]
// Gnd                            [Red]

// Screen
// 39   // SDA                    [Black]
// 40   // SCK                    [White]
// Gnd -> Ground wire to controls ground
// V++                            [Red]

// Normal buttons
// BUTTON_1 -> BUTTON_128
// Must call bleGamepad.press and bleGamepad.release

// Special Buttons
// START_BUTTON, SELECT_BUTTON, MENU_BUTTON, HOME_BUTTON, BACK_BUTTON, VOLUME_INC_BUTTON, VOLUME_DEC_BUTTON, VOLUME_MUTE_BUTTON
// Must call bleGamepad.pressSpecialButton and bleGamepad.releaseSpecialButton

// MAPPINGS
// Standard Guitar Hero (origional) controller...
// Green:1 Red:2 Yellow:4 Blue:3 Orange:5
// Whammy: Axis/Slider (0->4095)
// Tilt: Z Rotation/axis
// Strum: Hat Up + Hat Down
// ...shared with DPad - Hat Up + Hat Down + Hat Right + Hat Left
// Start: 7
// Select: 8

void RenderInput_Rectangle(Input* input);
void RenderInput_Icon(Input* input);
void RenderInput_DoubleIcon(Input* input);
void RenderInput_Text(Input* input);
void RenderInput_AnalogBar_Vert(Input* input);
void RenderInput_Hat(HatInput* hatInput);

static Input DigitalInputs[] = {
  // Pin, Text description, Bluetooth Input, Initial State (HIGH, LOW, -1), Render operation, x pos, y pos, width, height, Icon for if true, Icon for if false
  { PIN_D1, "1", BUTTON_1, -1, &BleGamepad::press, &BleGamepad::release, 0, RenderInput_Rectangle, 84, 30, 4, 5, 0, 0 },
  { PIN_D2, "2", BUTTON_2, -1, &BleGamepad::press, &BleGamepad::release, 0, RenderInput_Rectangle, 77, 30, 4, 5, 0, 0 },
  { PIN_D3, "3", BUTTON_4, -1, &BleGamepad::press, &BleGamepad::release, 0, RenderInput_Rectangle, 70, 30, 4, 5, 0, 0 },
  { PIN_D4, "4", BUTTON_3, -1, &BleGamepad::press, &BleGamepad::release, 0, RenderInput_Rectangle, 63, 30, 4, 5, 0, 0 },
  { PIN_D5, "5", BUTTON_5, -1, &BleGamepad::press, &BleGamepad::release, 0, RenderInput_Rectangle, 56, 30, 4, 5, 0, 0 },
  //{ PIN_D6, "Strum", BUTTON_6, -1, &BleGamepad::press, &BleGamepad::release, 0, RenderInput_Icon, 26, 25, 15, 15, Icon_Guitar2_CenterOn, Icon_Guitar2_CenterOff },
  // THIS MAY BE BUTTON_7
  { PIN_D10, "Start", BUTTON_7, -1, &BleGamepad::press, &BleGamepad::release, 0, RenderInput_Icon, 64, 20, 16, 5, Icon_Start, 0 },
  //{ PIN_D10, "Start", START_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 64, 20, 16, 5, Icon_Start, 0 },
  // THIS MAY BE BUTTON_8
  { PIN_D11, "Select", BUTTON_8, -1,&BleGamepad::press, &BleGamepad::release, 0, RenderInput_DoubleIcon, 63, 40, 19, 5, Icon_Select1, 0 },
  //{ PIN_D11, "Select", SELECT_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_DoubleIcon, 63, 40, 19, 5, Icon_Select1, 0 },
  { PIN_07_D04_A4, "Vol+", VOLUME_INC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolUp, 0 },
  { PIN_05_D02_A2, "Vol-", VOLUME_DEC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolDown, 0 },
  { PIN_06_D03_A3, "Mute", VOLUME_MUTE_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolMute, 0 },
  { PIN_D12, "Menu", MENU_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
  { PIN_D13, "Home", HOME_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
  { PIN_A5, "Back", BACK_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 }
};

int DigitalInputs_Count = sizeof(DigitalInputs) / sizeof(DigitalInputs[0]);

// Analog inputs...
// Left thumbstick is set via .setX() .setY()
// Right thumbstick is set via .setZ() and .setRZ
// Left trigger is set via .setRX()
// Right trigger is set via .setRY()
// Also .setSlider1 and .setSlider2

#define WhammyX 117
#define WhammyY 17
#define WhammyW 7
#define WhammyH 31

#define Enable_Slider1 1

static Input AnalogInputs[] = {
  //{ PIN_07_D04_A4, "Whammy", 0, -1, 0, 0, &BleGamepad::setX, RenderInput_AnalogBar_Vert, WhammyX + 2, WhammyY + 2, WhammyW - 4, WhammyH - 4, 0, 0 }
  { PIN_04_D01_A1, "Whammy", 0, -1, 0, 0, &BleGamepad::setSlider1, RenderInput_AnalogBar_Vert, WhammyX + 2, WhammyY + 2, WhammyW - 4, WhammyH - 4, 0, 0 }
};


int AnalogInputs_Count = sizeof(AnalogInputs) / sizeof(AnalogInputs[0]);

// Hat inputs...
// Assumes hats are used Hat1 -> 4

// Hat states, for all possible hats
// (all hats are passed to bleGamepad library, even if not used)
signed char HatValues[] = { 0, 0, 0, 0 };

static HatInput HatInputs[] = {
  { { PIN_D6, PIN_D7, PIN_D8, PIN_D9 }, "1", 0, -1, RenderInput_Hat, 98, 16, 15, 15, Icon_DPad_Neutral }
  //{ PIN_D6, PIN_D7, PIN_D8, PIN_D9, "Hat 1", BUTTON_1, -1, &BleGamepad::press, &BleGamepad::release, 0, RenderInput_Rectangle, 84, 30, 4, 5, 0, 0}
};

int HatInputs_Count = sizeof(HatInputs) / sizeof(HatInputs[0]);