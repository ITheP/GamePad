#include <optional>
#include <vector>
#include "Config.h"
#include "RenderInput.h"
#include "LED.h"
#include "IconMappings.h"
#include "Stats.h"
#include "LEDEffects.h"
//#include "LEDConfig.h"

#include "DeviceConfig.h"
#include <Screen.h>

// List of LED's we want cloning (lets you copy LED values between each other)
// e.g. when you might have multiple physical LED's that you want to share the same value, such as a light ring where you want the whole thing lit up at multiple points
IntPair LEDClones[] = { { LED_Status, LED_Status_Copy } };
int LEDClones_Count = sizeof(LEDClones) / sizeof(LEDClones[0]);

IconRun ControllerGfx[] = {
  { .StartIcon = Icon_Guitar2_T1, .Count = 6, .XPos = uiGuitar_xPos, .YPos = uiGuitar_yPos },
  { .StartIcon = Icon_Guitar2_B1, .Count = 6, .XPos = uiGuitar_xPos, .YPos = uiGuitar_yPos + 16 }
};

Input DigitalInput_Green = // Green button on guitar neck
  { .Pin = BUTTON_Green_PIN, .Label = "Green", .BluetoothInput = BUTTON_1, .DefaultValue = HIGH,
    .BluetoothPressOperation = &BleGamepad::press, .BluetoothReleaseOperation = &BleGamepad::release, .BluetoothSetOperation = NONE,
    .RenderOperation = RenderInput_Rectangle, .XPos = uiGuitar_xPos + 76, .YPos = uiGuitar_yPos + 13, .RenderWidth = 4, .RenderHeight = 5, .TrueIcon = NONE, .FalseIcon = NONE,
    .OnboardLED = { CRGB(0, 255, 0), true },
    .LEDConfig = new ExternalLEDConfig {
        //.LEDNumber = LED_Green,
        .LEDNumbers = { LED_DigitalTest,  (LED_DigitalTest+1), (LED_DigitalTest+2), (LED_DigitalTest+3), (LED_DigitalTest+4), (LED_DigitalTest+5), (LED_DigitalTest+6), (LED_DigitalTest+7),
          (LED_DigitalTest + 8),  (LED_DigitalTest+9), (LED_DigitalTest+10), (LED_DigitalTest+11), (LED_DigitalTest+12), (LED_DigitalTest+13), (LED_DigitalTest+14)
        },
        .PrimaryColour = { CRGB(0, 255, 0), true },
        .SecondaryColour = { CRGB(255, 0, 0), false },
        //.Effect = &DigitalEffects::Throb,
        ///.Effect = &DigitalArrayEffects::Rain,
        //.Effect = &DigitalArrayEffects::BlendedRain,
        //.Effect = &DigitalArrayEffects::SparkleTimeHue,
        .Effect = &DigitalArrayEffects::SparkleTimeBlend,
        .RunEffectConstantly = true,
        .Rate = 255.0, // sparkle -> 0.0069,  BlendedRain and Rain -> 0.069 //255.0 * 2
        .Chance = (uint32_t)(0.01 * 0xFFFF) // 10% chance of sparkle
    },
    .BluetoothIdOffset = 1
  };

Input DigitalInput_Red = // Red button on guitar neck
  { .Pin = BUTTON_Red_PIN, .Label = "Red", .BluetoothInput = BUTTON_2, .DefaultValue = HIGH,
    .BluetoothPressOperation = &BleGamepad::press, .BluetoothReleaseOperation = &BleGamepad::release, .BluetoothSetOperation = NONE,
    .RenderOperation = RenderInput_Rectangle, .XPos = uiGuitar_xPos + 69, .YPos = uiGuitar_yPos + 13, .RenderWidth = 4, .RenderHeight = 5, .TrueIcon = NONE, .FalseIcon = NONE,
    .OnboardLED = { CRGB(255, 0, 0), true },
    .LEDConfig = new ExternalLEDConfig{
      .LEDNumber = LED_Red,
      .PrimaryColour = { CRGB(255, 0, 0), true },
      .SecondaryColour = { CRGB(0, 0, 0), false }, 
      .Effect = &DigitalEffects::Pulse,
      .Rate = 255.0 * 2
    },
    .BluetoothIdOffset = 2
  };

Input DigitalInput_Yellow = // Yellow button on guitar neck
  { .Pin = BUTTON_Yellow_PIN, .Label = "Yellow", .BluetoothInput = BUTTON_4, .DefaultValue = HIGH,
    .BluetoothPressOperation = &BleGamepad::press, .BluetoothReleaseOperation = &BleGamepad::release, .BluetoothSetOperation = NONE,
    .RenderOperation = RenderInput_Rectangle, .XPos = uiGuitar_xPos + 62, .YPos = uiGuitar_yPos + 13, .RenderWidth = 4, .RenderHeight = 5, .TrueIcon = NONE, .FalseIcon = NONE,
    .OnboardLED = { CRGB(255, 255, 0), true },
    .LEDConfig = new ExternalLEDConfig {
        .LEDNumber = LED_Yellow,
        .PrimaryColour = { CRGB(255, 255, 0), true },
        .SecondaryColour = { CRGB(255, 255, 0), false },
        .Effect = &DigitalEffects::TimeHue,
        .Rate = 255.0 * 2
    },
    .BluetoothIdOffset = 3
  }; // Onboard LED set to slightly off yellow, then if red is pressed as well, you can kind of see it a bit

Input DigitalInput_Blue = // Blue button on guitar neck
  { .Pin = BUTTON_Blue_PIN, .Label = "Blue", .BluetoothInput = BUTTON_3, .DefaultValue = HIGH,
    .BluetoothPressOperation = &BleGamepad::press, .BluetoothReleaseOperation = &BleGamepad::release, .BluetoothSetOperation = NONE,
    .RenderOperation = RenderInput_Rectangle, .XPos = uiGuitar_xPos + 55, .YPos = uiGuitar_yPos + 13, .RenderWidth = 4, .RenderHeight = 5, .TrueIcon = NONE, .FalseIcon = NONE,
    .OnboardLED = { CRGB(0, 0, 255), true },
    .LEDConfig = new ExternalLEDConfig {
        .LEDNumber = LED_Blue,
        .PrimaryColour = { CRGB(0, 0, 255), true },
        .SecondaryColour = { CRGB(0, 0, 255), false },
        .Effect = &DigitalEffects::MoveRainbow,
        .Rate = 27.0
    },
    .BluetoothIdOffset = 4
  };

Input DigitalInput_Orange = // Orange button on guitar neck
  { .Pin = BUTTON_Orange_PIN, .Label = "Orange", .BluetoothInput = BUTTON_5, .DefaultValue = HIGH,
    .BluetoothPressOperation = &BleGamepad::press, .BluetoothReleaseOperation = &BleGamepad::release, .BluetoothSetOperation = NONE,
    .RenderOperation = RenderInput_Rectangle, .XPos = uiGuitar_xPos + 48, .YPos = uiGuitar_yPos + 13, .RenderWidth = 4, .RenderHeight = 5, .TrueIcon = NONE, .FalseIcon = NONE,
    .OnboardLED = { CRGB(255, 128, 0), true },
    .LEDConfig = new ExternalLEDConfig {
        .LEDNumber = LED_Orange,
        .PrimaryColour = { CRGB(255, 128, 0), true },
        .SecondaryColour = { CRGB(255, 128, 0), false }
    },
    .BluetoothIdOffset = 5
  }; // Onboard LED Slightly off colour again, so additional red looks different

Input DigitalInput_Start = // Start button on main body
  { .Pin = BUTTON_Start_PIN, .Label = "Start", .BluetoothInput = BUTTON_7, .DefaultValue = HIGH,
    .BluetoothPressOperation = &BleGamepad::press, .BluetoothReleaseOperation = &BleGamepad::release, .BluetoothSetOperation = NONE,
    .RenderOperation = RenderInput_Icon, .XPos = uiGuitar_xPos + 56, .YPos = uiGuitar_yPos + 3, .RenderWidth = 16, .RenderHeight = 5, .TrueIcon = Icon_Start, .FalseIcon = NONE
  };

Input DigitalInput_Select = // Select button on main body 
  { .Pin = BUTTON_Select_PIN, .Label = "Select", .BluetoothInput = BUTTON_8, .DefaultValue = HIGH,
    .BluetoothPressOperation = &BleGamepad::press, .BluetoothReleaseOperation = &BleGamepad::release, .BluetoothSetOperation = NONE,
    .RenderOperation = RenderInput_DoubleIcon, .XPos = uiGuitar_xPos + 55, .YPos = uiGuitar_yPos + 23, .RenderWidth = 19, .RenderHeight = 5, .TrueIcon = Icon_Select1, .FalseIcon = NONE
  };

Input DigitalInput_Tilt = // Tilt button on main body, or when guitar his tiled vertically
  { .Pin = BUTTON_Tilt_PIN, .Label = "Tilt", .BluetoothInput = BUTTON_9, .DefaultValue = HIGH,
    .BluetoothPressOperation = &BleGamepad::press, .BluetoothReleaseOperation = &BleGamepad::release, .BluetoothSetOperation = NONE,
    .RenderOperation = RenderInput_Icon, .XPos = uiGuitar_xPos + 91, .YPos = uiGuitar_yPos + 2, .RenderWidth = 7, .RenderHeight = 7, .TrueIcon = Icon_Tilt, .FalseIcon = NONE,
    .OnboardLED = { CRGB(0, 255, 255), true },
    .LEDConfig = new ExternalLEDConfig {
        .LEDNumber = LED_Tilt,
        .PrimaryColour = { CRGB(0, 255, 255), true },
        .SecondaryColour = { CRGB(0, 255, 255), false }
    }
  };

#define ENABLE_FLIP_SCREEN // Required if below is defined
//#define FLIP_SCREEN_TOGGLE 1 // FlipScreen can either toggle on and off with a button press (enable), or holding a button down sets its flipped state (disable)
Input DigitalInput_FlipScreen = // Lever on main body, will be flipped into a permanent on or off state, not just pressed
  { .Pin = BUTTON_FlipScreen_PIN, .Label = "Flip Screen", .BluetoothInput = NONE, .DefaultValue = -2,
    .BluetoothPressOperation = NONE, .BluetoothReleaseOperation = NONE, .BluetoothSetOperation = NONE,
    .RenderOperation = FlipScreen, .XPos = 0, .YPos = 0, .RenderWidth = 0, .RenderHeight = 0, .TrueIcon = NONE, .FalseIcon = NONE
  };

// DigitalInput array, collated list of all digital inputs (buttons) iterated over to check current state of each input
Input *DigitalInputs[] = {
  &DigitalInput_Green,
  &DigitalInput_Red,
  &DigitalInput_Yellow,   
  &DigitalInput_Blue,
  &DigitalInput_Orange,
  &DigitalInput_Start,
  &DigitalInput_Select,
  &DigitalInput_Tilt,
  &DigitalInput_FlipScreen

  // Other buttons of interest...
  //{ PIN_07_D04_A4, "Vol+", VOLUME_INC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolUp, 0 },
  //{ PIN_05_D02_A2, "Vol-", VOLUME_DEC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolDown, 0 },
  //{ PIN_06_D03_A3, "Mute", VOLUME_MUTE_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolMute, 0 },
  //{ PIN_D12, "Menu", MENU_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
  //{ PIN_D13, "Home", HOME_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
  //{ PIN_A5, "Back", BACK_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 }
};

// =============
// Analog inputs

#define Enable_Slider1 1

// Specific inputs we need references to
Input AnalogInputs_Whammy =
  { .Pin = ANALOG_Whammy_PIN, .Label = "Whammy", .BluetoothInput = NONE, .DefaultValue = -1,
    .BluetoothPressOperation = NONE, .BluetoothReleaseOperation = NONE, .BluetoothSetOperation = &BleGamepad::setSlider1,
    .RenderOperation = RenderInput_AnalogBar_Vert, .XPos = uiWhammyX + 2, .YPos = uiWhammyY + 2, .RenderWidth = uiWhammyW - 4, .RenderHeight = uiWhammyH - 4, .TrueIcon = NONE, .FalseIcon = NONE,
    .OnboardLED = { CRGB::Pink, true },
    .LEDConfig = new ExternalLEDConfig {
        .LEDNumbers = { LED_Whammy,  (LED_Whammy+1), (LED_Whammy+2), (LED_Whammy+3), (LED_Whammy+4), (LED_Whammy+5), (LED_Whammy+6), (LED_Whammy+7)
          },
        .PrimaryColour = { CRGB(255, 54, 96), true},
        .SecondaryColour = { CRGB::Green, false},
        //.Effect = &AnalogEffects::BlendedHue
        //.Effect = &AnalogEffects::SimpleSet_Fill
        //.Effect = &AnalogArrayEffects::BuildBlendToEnd
        //.Effect = &AnalogArrayEffects::SquishBlendToPoint
        .Effect = &AnalogArrayEffects::PointWithTail
        // .Effect = &LEDConfig::AnalogEffect_SimpleSet
        // .Effect = &LEDConfig::AnalogEffect_Throb
        // .Effect = &LEDConfig::AnalogEffect_Hue
        // .Effect = &LEDConfig::AnalogEffect_StartAtHue
        // .Effect = &LEDConfig::AnalogEffect_EndAtHue
        // .Effect = &LEDConfig::AnalogEffect_BlendedStartAtHue
        // .Effect = &LEDConfig::AnalogEffect_BlendedEndAtHue
    }
  };

Input *AnalogInputs[] = {
  &AnalogInputs_Whammy
};

// ==========
// Hat inputs

// Assumes hats are used Hat1 -> 4

// Hat states, for all possible hats
// (for simplicity, all hats are passed to bleGamepad library, even if not used)
// To define a hat without a pin/control point, set pin to NONE - e.g. might only want to use up/down but not left/right

unsigned char HatValues[] = { 0, 0, 0, 0 };

// Hat used for up/down strum bar
HatInput Hat0 =
  {
    .Pins = { NONE, HAT1_Right_PIN, NONE, HAT1_Left_PIN }, .Label = "1", .BluetoothHat = 0, .DefaultValue = 0,
    .RenderOperation = RenderInput_Hat, .XPos = -4, .YPos = 25, .RenderWidth = 15, .RenderHeight = 15, .StartIcon = Icon_DPad_Neutral,
    .ExtraOperation = {
        NONE,
        UpDownCountClick,
        NONE,
        NONE,
        NONE,
        UpDownCountClick,
        NONE,
        NONE,
        NONE
      },
    .CustomOperation = Custom_RenderHatStrumState,
    .OnboardLED = {
        {},
        { CRGB::Red, true },
        {},
        {},
        {},
        { CRGB::Green, true },
        {},
        {},
        {}
      },
    .LEDConfigs = {
        nullptr,
        new ExternalLEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Blue, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &HatEffects::MoveRainbow, .Rate = 255.0 * 2 }, //Throb, .Rate = 255.0 * 2 },
        nullptr,
        nullptr,
        nullptr,
        
        new ExternalLEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &HatEffects::MoveRainbow, .Rate = 27.0 }, //TimeHue, .Rate = 27.0 },
        //new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_SimpleSet, .Rate = 27.0 },
        //new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_Throb, .Rate = 27.0 },
        //new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_MoveRainbow, .Rate = 27.0 },
        //new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_Pulse, .Rate = 27.0 },
        //new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_TimeHue, .Rate = 27.0 },
        
        nullptr,
        nullptr,
        nullptr
      }
  };

HatInput *HatInputs[] = {
  &Hat0
};


// -----------------------------------------------------
// Array sizes

int ControllerGfx_RunCount = sizeof(ControllerGfx) / sizeof(ControllerGfx[0]);
int AnalogInputs_Count = sizeof(AnalogInputs) / sizeof(AnalogInputs[0]);
int HatInputs_Count = sizeof(HatInputs) / sizeof(HatInputs[0]);
int DigitalInputs_Count = sizeof(DigitalInputs) / sizeof(DigitalInputs[0]);

// Special case code specific to this controller

// HAT has secondary rendering, with up/down also mapped to strum bar up/down which we want to visualise
void Custom_RenderHatStrumState(HatInput *hatInput) {
    // Special Case drawing of extra HAT interaction - the digital d-pad up/down also map to the strum bar up/down
    Display.fillRect(26, 25, 15, 15, C_BLACK);
    char c;
    if (hatInput->IndividualStates[1] == LOW)
      c = Icon_Guitar2_CenterBottom;
    else if (hatInput->IndividualStates[3] == LOW)
      c = Icon_Guitar2_CenterTop;
    else
      c = Icon_Guitar2_CenterOff;

    RRE.drawChar(26, 25, c);
}