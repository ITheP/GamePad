#pragma once

#include <optional>
#include <vector>
#include "Config.h"
#include <BleGamepad.h>
#include <FastLED.h>
#include "LED.h"
#include "stats.h"
//#include "GamePad.h"
//class Stats;

typedef void (BleGamepad::*BleGamepadFunctionPointer)(uint8_t);
typedef void (BleGamepad::*BleGamepadFunctionPointerInt)(int16_t);

typedef void (*LEDEffectFunctionPointerOld)(Input*, float);
typedef void (*LEDHatEffectFunctionPointer)(HatInput*, int, float);

typedef struct IntPair {
  int A;
  int B;
} IntPair;

typedef struct IconRun {
  unsigned char StartIcon;
  int Count;
  int XPos;
  int YPos;
} IconRun;

typedef struct State {
  int StateJustChanged; // = false;       // Used for things like LED effects that only want to change a value when a state change happens
  unsigned long StateChangedWhen;
  int StateJustChangedLED;                // Sticks around till LED processing is done, then set to false again
  int16_t Value;
  int16_t PreviousValue;
} State;

// General input (Digital and Analog - e.g. buttons)
typedef struct Input {
  uint8_t Pin;
  const char* Label;
  int BluetoothInput;
  int16_t DefaultValue;

  BleGamepadFunctionPointer BluetoothPressOperation;
  BleGamepadFunctionPointer BluetoothReleaseOperation; 
  BleGamepadFunctionPointerInt BluetoothSetOperation;

  void (*RenderOperation)(struct Input*);       // How input is rendered to the screen
  void (*CustomOperation)();               // Custom/specific operation, code may be within controller .cpp

  int XPos;
  int YPos;
  int RenderWidth;
  int RenderHeight;

  unsigned char TrueIcon;
  unsigned char FalseIcon;

  Stats *Statistics;                                 // Stats for this input, if required
                                                // Below LED's are ALWAYS used/set, non existence defaults to {0,0,0,false} which will turn things off
  LED OnboardLED;                               // Onboard LED is merged into other Onboard LED colours to create a `final combined colour`. Max of each R,G,B component generally gets used.
  ExternalLEDConfig* LEDConfig;

  int BluetoothIdOffset;                        // Set > 0 to enable this Input for Bluetooth Id override inclusion on startup
  State ValueState;
} Input;

// DPad input - Hat's have 4 inputs for Up/Down/Left/Right
typedef struct HatInput {
  uint8_t Pins[4];
  const char* Label;
  int BluetoothHat;                             // 0->3
  int16_t DefaultValue;

  void (*RenderOperation)(struct HatInput*);    // Single general operation - does the job
  int XPos;
  int YPos;
  int RenderWidth;
  int RenderHeight;
  unsigned char StartIcon;                      // Represents first icon in list of mapped icons. Should be 9 - Neutral, Up, UpRight, Right, DownRight, Down, DownLeft, Left, UpLeft

  void (*ExtraOperation[9])(struct HatInput*);  // Extra operations that can run if neutral/up/up right/right/down right/down/down left/left/up left
  void (*CustomOperation)(struct HatInput*);    // Controller specific operation, code within controller .cpp

  Stats *Statistics[9];

  LED OnboardLED[9];

  // ExternalLEDNumbers are separate to LEDConfigs - multiple LEDConfigs might point back to the same ExternalLED (e.g. single one that shows different colour for each hat position)
  ExternalLEDConfig* LEDConfigs[9];

  // Following don't need initialising
  long IndividualStateChangedWhen[4];
  uint8_t PreviousValuePin;
  uint8_t IndividualStates[5];
  
  State ValueState;
} HatInput;