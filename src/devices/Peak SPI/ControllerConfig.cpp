#include <optional>
#include <vector>
#include "Config.h"
#include "Defines.h"
#include "RenderInput.h"
#include "LED.h"
#include "IconMappings.h"
#include "Stats.h"
#include "LEDEffects.h"
// #include "LEDConfig.h"

#include "DeviceConfig.h"
#include <Screen.h>
#include "Menus.h"

char ControllerDeviceNameType[] = "Guitar";
char ControllerType[] = "Guitar Controller";
char ModelNumber[] = "Guitar 1.0";
char FirmwareRevision[] = "1.0";
char HardwareRevision[] = "1.0";
char SoftwareRevision[] = "1.0";

// List of LED's we want cloning (lets you copy LED values between each other)
// e.g. when you might have multiple physical LED's that you want to share the same value, such as a light ring where you want the whole thing lit up at multiple points
IntPair LEDClones[] = {
    {LED_Green, LED_Green_Neck},
    {LED_Red, LED_Red_Neck},
    {LED_Yellow, LED_Yellow_Neck},
    {LED_Blue, LED_Blue_Neck},
    {LED_Orange, LED_Orange_Neck}
}; //{ LED_Status, LED_Status_Copy } };
int LEDClones_Count = sizeof(LEDClones) / sizeof(LEDClones[0]);

IconRun ControllerGfx[] = {
    {.StartIcon = Icon_Guitar2_T1, .Count = 6, .XPos = uiGuitar_xPos, .YPos = uiGuitar_yPos},
    {.StartIcon = Icon_Guitar2_B1, .Count = 6, .XPos = uiGuitar_xPos, .YPos = uiGuitar_yPos + 16}};

// Usage statistics
// Stats used in various places, including any additional chain of other stats
// IMPORTANT: PrefsKey max length is 9 chars. This is used for saving stats into Preferences.
Stats Stats_Neck("Neck", "Neck");
Stats Stats_Green("Green", "Green", &Stats_Neck);
Stats Stats_Red("Red", "Red", &Stats_Neck);
Stats Stats_Yellow("Yellow", "Yellow", &Stats_Neck);
Stats Stats_Blue("Blue", "Blue", &Stats_Neck);
Stats Stats_Orange("Orange", "Orange", &Stats_Neck);

Stats Stats_Start_LongPress("Start Long Press", "StartLP");

Stats Stats_StrumBar("Strum Bar", "StrumBar");
Stats Stats_HatUp("Strum Up", "StrumUp", &Stats_StrumBar);
Stats Stats_HatDown("Strum Down", "StrumDown", &Stats_StrumBar);

// Collated list of all stats to easily go through them to load/save/clear and run once per second and updates etc.
Stats *AllStats[] = {
    &Stats_Neck,
    &Stats_Green,
    &Stats_Red,
    &Stats_Yellow,
    &Stats_Blue,
    &Stats_Orange,
    &Stats_StrumBar,
    &Stats_HatUp,
    &Stats_HatDown,
    &Stats_Start_LongPress};
int AllStats_Count = sizeof(AllStats) / sizeof(AllStats[0]);

// ToDo: RenderStats - what gets rendered where (icon/text, position, stats value, left/mid/right aligned)

// Early boot - config initiation and menu buttons
// Very specific, low level handling
// before device has booted (generally used to configure WiFi)

// Assumes this pin is defined in Digital_Input collection - i.e. will be enabled for reading. If not, may need extra code to enable.
uint8_t BootPin_StartInConfiguration = BUTTON_Select_PIN;

// Assumes these pins are defined in Digital_Input collection - i.e. will be enabled for reading. If not, may need extra code to enable.
Input DigitalInput_Config_Up = {.Pin = HAT1_Up_PIN, .Label = DIGITALINPUT_CONFIG_UP_LABEL};
Input DigitalInput_Config_Down = {.Pin = HAT1_Down_PIN, .Label = DIGITALINPUT_CONFIG_DOWN_LABEL};
Input DigitalInput_Config_Select = {.Pin = BUTTON_Green_PIN, .Label = DIGITALINPUT_CONFIG_SELECT_LABEL};
Input DigitalInput_Config_Back = {.Pin = BUTTON_Red_PIN, .Label = DIGITALINPUT_CONFIG_BACK_LABEL};

// Input DigitalInput_Config_MenuUp = { .Pin = HAT1_Up_PIN, .Label = "Strum Up", .CustomOperationPressed = &Menus::Config_UpPressed, .CustomOperationReleased = &Menus::Config_UpReleased };
// Input DigitalInput_Config_MenuDown = { .Pin = HAT1_Down_PIN, .Label = "Strum Down", .CustomOperationPressed = &Menus::Config_DownPressed, .CustomOperationReleased = &Menus::Config_DownReleased };
// Input DigitalInput_Config_Select = { .Pin = BUTTON_Green_PIN, .Label = "Green Button", .CustomOperationPressed = &Menus::Config_SelectPressed, .CustomOperationReleased = &Menus::Config_SelectReleased };
// Input DigitalInput_Config_Back = { .Pin = BUTTON_Red_PIN, .Label = "Red Button", .CustomOperationPressed = &Menus::Config_BackPressed, .CustomOperationReleased = &Menus::Config_BackReleased };

Input *DigitalInputs_ConfigMenu[] = {
    &DigitalInput_Config_Up,
    &DigitalInput_Config_Down,
    &DigitalInput_Config_Select,
    &DigitalInput_Config_Back};

// =============
// Analog inputs
// Defined before Digital Inputs incase Digital Inputs need to reference them as VirtualPinInputs

#define Enable_Slider1 1

// Analog Button -
// Specific inputs we need references to
// On @ value e.g. 4000 -> 2000 = trigger
// Off @ value e.g. 2000 -> 3000 = release
// Secondary AnalogInput
// ...Straight Analog input only enabled/pressed when on/off in Analog button is in pressed state (i.e. emulate values going into it)
// ...Analog wobble engage - when button is on, and variance in value is above a certain threshold it enables the secondary AnalogInput then passes through the value (e.g. simulates whammy bar). Include a threshold trigger e.g. to push harder to engage, and then for the e.g. wobble a scaling factor -> the secondary analoginput's range

// AnalogInput Input type needs a `Virtual Trigger` reference
// Process AnalogButtons first and they set a virtual Analog value against them
// Then when processing AnalogInputs, if input = virtual (i.e. has a reference to an AnalogButton), it gets its value from the AnalogButton instead of the pin reading
// HAVE TO MAKE SURE MULTIPLE INPUTS TO SAME TARGET DONT OVERRIDE EACH OTHER IF NOT IN USE

// Test Virtual Source - maps test analog to triggered red button press + whammy bar when triggered
// SO WE NEED TO MAP MULTIPLE OF THESE FOR THE RED ORANGE GREEN BLUE YELLOW

// Sensitive Hall Switch
// .MinAnalogValue = 2500,
// .MaxAnalogValue = 4095,
// .TriggerOnValue = 2800,
// .TriggerOffValue = 2500,

// Cheap Hall Switch
// .MinAnalogValue = 2200,
// .MaxAnalogValue = 2900,
// .TriggerOnValue = 2600,
// .TriggerOffValue = 2300,

// TODO: Another AnalogInput Virtual Type
// On if in range / off if out of range
// so can map capacitive slider into button press
// May be best to have a new input type.... MappedAnalogInput
// then we can read it once, have a series of mapped values e.g. Analoge to array of on/off states
// where it iterates through each boundary range
// e.g. Button 1 = 4096-3800, Button 2= 3799-3600
// etc.
// We always want to scan all buttons to reset ones that are being unset
// To make this work we simply...
// MappedAnalogInput is proccessed FIRST to populate a value
// AnalogInput_Virtual for each

//hmmmmmmm
//maybe we can just...
//Input AnalogInputs_Virtual_MappedTriggeredGreen =
//.pin = the analog pin (doesnt matter if it's read more than once)
//.VirtualPinMode = MappedAnalogToPressed
//.MinAnalogValue and MaxAnalogValue give the range its considered on
// TODO
// The VirtualPinMode on the digital - make sure default is ok else needs to be a value for the existing analog inputs that just reads the on/off value
// and then extra entries to get the new MappedTriggeredGreen value
// Hmmm
// OK the VirtualPinMode keep as is its about how virtual values are USED not valculated
// need a new CalculationMode which is 0 by default and everything uses
// but for our new MappedTriggered... variant it's flagged 
//  MappedAnalogToPressed = 2                  // For analog inputs, maps the value from the virtual input's analog value range to digital on/off

// Note if you wanted to optimise rereading same pin, then it saves the existing value doesnt it already
// if we saved the frame it was saved on, we can just compaire frame number to see if already read and if different read it else reuse it
// really depends on overhead of re-reading an input multiple times

// Virtual AnalogInput effectively takes a physical input, and turns it into a virtual source for other inputs
// e.g. an input that acts as both a digital on/off input and an analoge input
// AnalogInputs and DigitalInputs can reference this Virtual input as a VirtualPinInput source and grab values
Input AnalogInputs_Virtual_TriggeredGreen =
    {
        .Pin = BUTTON_Green_PIN,
        .Label = "Virtual Trig. Green + A. Whammy",
        .BluetoothInput = NONE,
        .DefaultValue = NOT_PRESSED,
        .DefaultAnalogValue = -1,
        .MinAnalogValue = 2200,
        .MaxAnalogValue = 2900,
        .TriggerOnValue = 2600,
        .TriggerOffValue = 2300,
        .BluetoothPressOperation = NONE,
        .BluetoothReleaseOperation = NONE,
        .BluetoothSetOperation = NONE,
        .RenderOperation = NONE};

Input AnalogInputs_Virtual_TriggeredRed =
    {
        .Pin = BUTTON_Red_PIN,
        .Label = "Virtual Trig. Red + A. Whammy",
        .BluetoothInput = NONE,
        .DefaultValue = NOT_PRESSED,
        .DefaultAnalogValue = -1,
        .MinAnalogValue = 2200,
        .MaxAnalogValue = 2900,
        .TriggerOnValue = 2600,
        .TriggerOffValue = 2300,
        .BluetoothPressOperation = NONE,
        .BluetoothReleaseOperation = NONE,
        .BluetoothSetOperation = NONE,
        .RenderOperation = NONE};

Input AnalogInputs_Virtual_TriggeredYellow =
    {
        .Pin = BUTTON_Yellow_PIN,
        .Label = "Virtual Trig. Yellow + A. Whammy",
        .BluetoothInput = NONE,
        .DefaultValue = NOT_PRESSED,
        .DefaultAnalogValue = -1,
        .MinAnalogValue = 2200,
        .MaxAnalogValue = 2900,
        .TriggerOnValue = 2600,
        .TriggerOffValue = 2300,
        .BluetoothPressOperation = NONE,
        .BluetoothReleaseOperation = NONE,
        .BluetoothSetOperation = NONE,
        .RenderOperation = NONE};

Input AnalogInputs_Virtual_TriggeredBlue =
    {
        .Pin = BUTTON_Blue_PIN,
        .Label = "Virtual Trig. Blue + A. Whammy",
        .BluetoothInput = NONE,
        .DefaultValue = NOT_PRESSED,
        .DefaultAnalogValue = -1,
        .MinAnalogValue = 2200,
        .MaxAnalogValue = 2900,
        .TriggerOnValue = 2600,
        .TriggerOffValue = 2300,
        .BluetoothPressOperation = NONE,
        .BluetoothReleaseOperation = NONE,
        .BluetoothSetOperation = NONE,
        .RenderOperation = NONE};

Input AnalogInputs_Virtual_TriggeredOrange =
    {
        .Pin = BUTTON_Orange_PIN,
        .Label = "Virtual Trig. Orange + A. Whammy",
        .BluetoothInput = NONE,
        .DefaultValue = NOT_PRESSED,
        .DefaultAnalogValue = -1,
        .MinAnalogValue = 2200,
        .MaxAnalogValue = 2900,
        .TriggerOnValue = 2600,
        .TriggerOffValue = 2300,
        .BluetoothPressOperation = NONE,
        .BluetoothReleaseOperation = NONE,
        .BluetoothSetOperation = NONE,
        .RenderOperation = NONE};

// Specific inputs we need references to
// So the Whammy input can get its input EITHER from the actual pin, or drag in a value (if being provided) from the Virtual Pin.
Input AnalogInputs_Whammy =
    {
        .Pin = ANALOG_Whammy_PIN,
        .VirtualPinInputs = {&AnalogInputs_Virtual_TriggeredGreen, &AnalogInputs_Virtual_TriggeredRed, &AnalogInputs_Virtual_TriggeredYellow, &AnalogInputs_Virtual_TriggeredBlue, &AnalogInputs_Virtual_TriggeredOrange},
        .VirtualPinMode = VirtualPinModes::RequireValueToBePressed, // Only take analog values from virtual pin when they are also in a pressed state
        .Label = "Whammy",
        .BluetoothInput = NONE,
        .DefaultAnalogValue = -1,
        .MinAnalogValue = 2300,
        .MaxAnalogValue = 3500,
        .BluetoothPressOperation = NONE,
        .BluetoothReleaseOperation = NONE,
        .BluetoothSetOperation = &BleGamepad::setSlider1,
        .RenderOperation = RenderInput_AnalogBar_Vert,
        .XPos = uiWhammyX + 2,
        .YPos = uiWhammyY + 2,
        .RenderWidth = uiWhammyW - 4,
        .RenderHeight = uiWhammyH - 4,
        .TrueIcon = NONE,
        .FalseIcon = NONE,
        .OnboardLED = {CRGB::Pink, true}
        // .LEDConfig = new ExternalLEDConfig {
        //     .LEDNumbers = { LED_Whammy,  (LED_Whammy+1), (LED_Whammy+2), (LED_Whammy+3), (LED_Whammy+4), (LED_Whammy+5), (LED_Whammy+6), (LED_Whammy+7)
        //       },
        //     .PrimaryColour = { CRGB(255, 54, 96), true},
        //     .SecondaryColour = { CRGB::Green, false},
        //     //.Effect = &AnalogEffects::BlendedHue
        //     //.Effect = &AnalogEffects::SimpleSet_Fill
        //     //.Effect = &AnalogArrayEffects::BuildBlendToEnd
        //     //.Effect = &AnalogArrayEffects::SquishBlendToPoint
        //     .Effect = &AnalogArrayEffects::PointWithTail
        //     // .Effect = &LEDConfig::AnalogEffect_SimpleSet
        //     // .Effect = &LEDConfig::AnalogEffect_Throb
        //     // .Effect = &LEDConfig::AnalogEffect_Hue
        //     // .Effect = &LEDConfig::AnalogEffect_StartAtHue
        //     // .Effect = &LEDConfig::AnalogEffect_EndAtHue
        //     // .Effect = &LEDConfig::AnalogEffect_BlendedStartAtHue
        //     // .Effect = &LEDConfig::AnalogEffect_BlendedEndAtHue
        // }
};

// Input AnalogInputs_VirtualWhammyTest =
// {
//         .VirtualPin = &AnalogInputs_Test_TriggeredWobble,
//         .Label = "Virtual Whammy Test",
//         .BluetoothInput = NONE,
//         .DefaultValue = -1,
//         .MinAnalogValue = 2800,
//         .MaxAnalogValue = 2000,
//         .BluetoothPressOperation = NONE,
//         .BluetoothReleaseOperation = NONE,
//         .BluetoothSetOperation = &BleGamepad::setSlider1,
//         .AnalogRenderOperation = RenderInput_AnalogBar_Vert,
//         .XPos = uiWhammyX + 2,
//         .YPos = uiWhammyY + 2,
//         .RenderWidth = uiWhammyW - 4,
//         .RenderHeight = uiWhammyH - 4,
//         .TrueIcon = NONE,
//         .FalseIcon = NONE,
//         .OnboardLED = {CRGB::Pink, true}
// };

Input *AnalogInputs[] = {
    &AnalogInputs_Whammy,
    &AnalogInputs_Virtual_TriggeredGreen,
    &AnalogInputs_Virtual_TriggeredRed,
    &AnalogInputs_Virtual_TriggeredYellow,
    &AnalogInputs_Virtual_TriggeredBlue,
    &AnalogInputs_Virtual_TriggeredOrange};

// Digital inputs

Input DigitalInput_Green = // Green button on guitar neck
    {
        .Pin = NONE, // BUTTON_Green_PIN,
        .VirtualPinInputs = {&AnalogInputs_Virtual_TriggeredGreen},
        .Label = "Green",
        .BluetoothInput = BUTTON_1,
        .DefaultValue = HIGH,
        .BluetoothPressOperation = &BleGamepad::press,
        .BluetoothReleaseOperation = &BleGamepad::release,
        .BluetoothSetOperation = NONE,
        .RenderOperation = RenderInput_Rectangle,
        .XPos = uiGuitar_xPos + 76,
        .YPos = uiGuitar_yPos + 13,
        .RenderWidth = 4,
        .RenderHeight = 5,
        .TrueIcon = NONE,
        .FalseIcon = NONE,
        .Statistics = &Stats_Green,
        .OnboardLED = {CRGB(0, 255, 0), true},
        .LEDConfig = new ExternalLEDConfig {
            .LEDNumber = LED_Green,
            .PrimaryColour = { CRGB(0, 255, 0), true },
            .SecondaryColour = { CRGB(96, 96, 96), true },
            .Effect = &AnalogEffects::ConstrainedSimpleSet,
            .RunEffectConstantly = true,
         },
        .ProfileId = 1};

Input DigitalInput_Red = // Red button on guitar neck
    {
        .Pin = NONE, // BUTTON_Red_PIN,
        .VirtualPinInputs = {&AnalogInputs_Virtual_TriggeredRed},
        .Label = "Red",
        .BluetoothInput = BUTTON_2,
        .DefaultValue = HIGH,
        .BluetoothPressOperation = &BleGamepad::press,
        .BluetoothReleaseOperation = &BleGamepad::release,
        .BluetoothSetOperation = NONE,
        .RenderOperation = RenderInput_Rectangle,
        .XPos = uiGuitar_xPos + 69,
        .YPos = uiGuitar_yPos + 13,
        .RenderWidth = 4,
        .RenderHeight = 5,
        .TrueIcon = NONE,
        .FalseIcon = NONE,
        .Statistics = &Stats_Red,
        .OnboardLED = {CRGB(255, 0, 0), true},
        .LEDConfig = new ExternalLEDConfig {
            .LEDNumber = LED_Red,
            .PrimaryColour = { CRGB(255, 0, 0), true },
            .SecondaryColour = { CRGB(96, 96, 96), true },
            .Effect = &AnalogEffects::SimpleSet,
            .RunEffectConstantly = true,
         },
        .ProfileId = 2};

Input DigitalInput_Yellow = // Yellow button on guitar neck
    {
        .Pin = NONE, // BUTTON_Yellow_PIN,
        .VirtualPinInputs = {&AnalogInputs_Virtual_TriggeredYellow},
        .Label = "Yellow",
        .BluetoothInput = BUTTON_4,
        .DefaultValue = HIGH,
        .BluetoothPressOperation = &BleGamepad::press,
        .BluetoothReleaseOperation = &BleGamepad::release,
        .BluetoothSetOperation = NONE,
        .RenderOperation = RenderInput_Rectangle,
        .XPos = uiGuitar_xPos + 62,
        .YPos = uiGuitar_yPos + 13,
        .RenderWidth = 4,
        .RenderHeight = 5,
        .TrueIcon = NONE,
        .FalseIcon = NONE,
        .Statistics = &Stats_Yellow,
        .OnboardLED = {CRGB(255, 255, 0), true},
        .LEDConfig = new ExternalLEDConfig {
            .LEDNumber = LED_Yellow,
            .PrimaryColour = { CRGB(255, 255, 0), true },
            .SecondaryColour = { CRGB(96, 96, 96), true },
            .Effect = &AnalogEffects::SimpleSet,
            .RunEffectConstantly = true,
         },
        .ProfileId = 3}; // Onboard LED set to slightly off yellow, then if red is pressed as well, you can kind of see it a bit

Input DigitalInput_Blue = // Blue button on guitar neck
    {
        .Pin = NONE, // BUTTON_Blue_PIN,
        .VirtualPinInputs = {&AnalogInputs_Virtual_TriggeredBlue},
        .Label = "Blue",
        .BluetoothInput = BUTTON_3,
        .DefaultValue = HIGH,
        .BluetoothPressOperation = &BleGamepad::press,
        .BluetoothReleaseOperation = &BleGamepad::release,
        .BluetoothSetOperation = NONE,
        .RenderOperation = RenderInput_Rectangle,
        .XPos = uiGuitar_xPos + 55,
        .YPos = uiGuitar_yPos + 13,
        .RenderWidth = 4,
        .RenderHeight = 5,
        .TrueIcon = NONE,
        .FalseIcon = NONE,
        .Statistics = &Stats_Blue,
        .OnboardLED = {CRGB(0, 0, 255), true},
        .LEDConfig = new ExternalLEDConfig {
            .LEDNumber = LED_Blue,
            .PrimaryColour = { CRGB(0, 0, 255), true },
            .SecondaryColour = { CRGB(96, 96, 96), true },
            .Effect = &AnalogEffects::SimpleSet,
            .RunEffectConstantly = true,
         },
        .ProfileId = 4};

Input DigitalInput_Orange = // Orange button on guitar neck
    {
        .Pin = NONE, // BUTTON_Orange_PIN,
        .VirtualPinInputs = {&AnalogInputs_Virtual_TriggeredOrange},
        .Label = "Orange",
        .BluetoothInput = BUTTON_5,
        .DefaultValue = HIGH,
        .BluetoothPressOperation = &BleGamepad::press,
        .BluetoothReleaseOperation = &BleGamepad::release,
        .BluetoothSetOperation = NONE,
        .RenderOperation = RenderInput_Rectangle,
        .XPos = uiGuitar_xPos + 48,
        .YPos = uiGuitar_yPos + 13,
        .RenderWidth = 4,
        .RenderHeight = 5,
        .TrueIcon = NONE,
        .FalseIcon = NONE,
        .Statistics = &Stats_Orange,
        .OnboardLED = {CRGB(255, 128, 0), true},
        .LEDConfig = new ExternalLEDConfig {
            .LEDNumber = LED_Orange,
            .PrimaryColour = { CRGB(255, 128, 0), true },
            .SecondaryColour = { CRGB(96, 96, 96), true },
            .Effect = &AnalogEffects::SimpleSet,
            .RunEffectConstantly = true,
         },
        .ProfileId = 5}; // Onboard LED Slightly off colour again, so additional red looks different

// TEST - same as select but with some LED
Input DigitalInput_Start_LongPress = // Select button on main body
    {
        .Pin = BUTTON_Start_PIN,
        .Label = "Start Long Press",
        .BluetoothInput = NONE,
        .DefaultValue = HIGH,
        .BluetoothPressOperation = &BleGamepad::press,
        .BluetoothReleaseOperation = &BleGamepad::release,
        .BluetoothSetOperation = NONE,
        .CustomOperationPressed = Menus::ToggleMenuMode,
        .CustomOperationReleased = NONE,
        .RenderOperation = RenderInput_Icon,
        .XPos = uiGuitar_xPos + 56,
        .YPos = uiGuitar_yPos + 3,
        .RenderWidth = 16,
        .RenderHeight = 5,
        .TrueIcon = Icon_Menu,
        .FalseIcon = NONE,
        .Statistics = &Stats_Start_LongPress,
        .OnboardLED = {CRGB(255, 255, 255), true}
        //     .LEDConfig = new ExternalLEDConfig {
        //     .LEDNumber = LED_Orange,
        //     .PrimaryColour = { CRGB(255, 255, 255), true },
        //     .SecondaryColour = { CRGB(255, 0, 255), false }
        // }
};

Input DigitalInput_Start = // Start button on main body
    {
        .Pin = BUTTON_Start_PIN,
        .Label = "Start",
        .BluetoothInput = BUTTON_7,
        .DefaultValue = HIGH,
        .BluetoothPressOperation = &BleGamepad::press,
        .BluetoothReleaseOperation = &BleGamepad::release,
        .BluetoothSetOperation = NONE,
        .RenderOperation = RenderInput_Icon,
        .XPos = uiGuitar_xPos + 56,
        .YPos = uiGuitar_yPos + 3,
        .RenderWidth = 16,
        .RenderHeight = 5,
        .TrueIcon = Icon_Start,
        .FalseIcon = NONE,
        .OnboardLED = {CRGB(255, 128, 0), true},
        .LongPressTiming = 1000 * 1000,                       // 1sec = 1000ms = 1000000us
        .LongPressChildInput = &DigitalInput_Start_LongPress, // Long press on Start button will trigger Select Long Press
        .ShortPressReleaseTime = 100 * 1000                   // When short press is triggered, rather than instantly going to off state, keep on for this many ms to allow for e.g. LED to show reasonably clearly
};

//   // TEST - same as select but with some LED
// Input DigitalInput_Select_LongPress = // Select button on main body
//   { .Pin = BUTTON_Select_PIN, .Label = "Select Long Press", .BluetoothInput = NONE, .DefaultValue = HIGH,
//     .BluetoothPressOperation = NONE, .BluetoothReleaseOperation = NONE, .BluetoothSetOperation = NONE,
//     .CustomOperationPressed = Menus::ToggleMenuMode, .CustomOperationReleased = NONE,
//     .RenderOperation = RenderInput_DoubleIcon, .XPos = uiGuitar_xPos + 55, .YPos = uiGuitar_yPos + 23, .RenderWidth = 19, .RenderHeight = 5, .TrueIcon = Icon_Select1, .FalseIcon = NONE,
//     .OnboardLED = { CRGB(255, 255, 255), true } // Long press on Start button will trigger Select Long Press
//     //     .LEDConfig = new ExternalLEDConfig {
//     //     .LEDNumber = LED_Orange,
//     //     .PrimaryColour = { CRGB(255, 255, 255), true },
//     //     .SecondaryColour = { CRGB(255, 0, 255), false }
//     // }
//   };

Input DigitalInput_Select = // Select button on main body
    {
        .Pin = BUTTON_Select_PIN,
        .Label = "Select",
        .BluetoothInput = BUTTON_8,
        .DefaultValue = HIGH,
        .BluetoothPressOperation = &BleGamepad::press,
        .BluetoothReleaseOperation = &BleGamepad::release,
        .BluetoothSetOperation = NONE,
        .RenderOperation = RenderInput_DoubleIcon,
        .XPos = uiGuitar_xPos + 55,
        .YPos = uiGuitar_yPos + 23,
        .RenderWidth = 19,
        .RenderHeight = 5,
        .TrueIcon = Icon_Select1,
        .FalseIcon = NONE,
        .OnboardLED = {CRGB(255, 128, 0), true},
        //.LongPressTiming = 1000 * 1000, // 1sec = 1000ms = 1000000us
        // .LongPressChildInput = &DigitalInput_Select_LongPress,
        //.ShortPressReleaseTime = 250 * 1000
};

Input DigitalInput_Tilt = // Tilt button on main body, or when guitar his tiled vertically
    {
        .Pin = BUTTON_Tilt_PIN,
        .Label = "Tilt",
        .BluetoothInput = BUTTON_9,
        .DefaultValue = HIGH,
        .BluetoothPressOperation = &BleGamepad::press,
        .BluetoothReleaseOperation = &BleGamepad::release,
        .BluetoothSetOperation = NONE,
        .RenderOperation = RenderInput_Icon,
        .XPos = uiGuitar_xPos + 91,
        .YPos = uiGuitar_yPos + 2,
        .RenderWidth = 7,
        .RenderHeight = 7,
        .TrueIcon = Icon_Tilt,
        .FalseIcon = NONE,
        .OnboardLED = {CRGB(0, 255, 255), true}
        // .LEDConfig = new ExternalLEDConfig {
        //     .LEDNumber = LED_Tilt,
        //     .PrimaryColour = { CRGB(0, 255, 255), true },
        //     .SecondaryColour = { CRGB(0, 255, 255), false }
        // }
};

#define ENABLE_FLIP_SCREEN // Required if below is defined
// #define FLIP_SCREEN_TOGGLE 1 // FlipScreen can either toggle on and off with a button press (enable), or holding a button down sets its flipped state (disable)
Input DigitalInput_FlipScreen = // Lever on main body, will be flipped into a permanent on or off state, not just pressed
    {
        .Pin = BUTTON_FlipScreen_PIN,
        .Label = "Flip Screen",
        .BluetoothInput = NONE,
        .DefaultValue = -2,
        .BluetoothPressOperation = NONE,
        .BluetoothReleaseOperation = NONE,
        .BluetoothSetOperation = NONE,
#ifdef CLEAR_STATS_ON_FLIP
        .CustomOperationPressed = ResetAllCurrentStats, // When screen flips, we reset all the stats
#endif
        .RenderOperation = FlipScreen,
        .XPos = 0,
        .YPos = 0,
        .RenderWidth = 0,
        .RenderHeight = 0,
        .TrueIcon = NONE,
        .FalseIcon = NONE
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
    &DigitalInput_FlipScreen,

    // Other buttons of interest...
    //{ PIN_07_D04_A4, "Vol+", VOLUME_INC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolUp, 0 },
    //{ PIN_05_D02_A2, "Vol-", VOLUME_DEC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolDown, 0 },
    //{ PIN_06_D03_A3, "Mute", VOLUME_MUTE_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolMute, 0 },
    //{ PIN_D12, "Menu", MENU_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
    //{ PIN_D13, "Home", HOME_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
    //{ PIN_A5, "Back", BACK_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 }

    // Long press inputs
    &DigitalInput_Start_LongPress
    //&DigitalInput_Select_LongPress
};

// // Secondary Digital Inputs, used for long press operations
// // These are NOT iterated over
// Input *LongPressDigitalInputs[] = {
// };

// // Should include both DigitalInputs AND LongPressDigitalInputs - referenced by e.g. LED code to iterate through all digital inputs
// Input *AllDigitalInputs[] = {
//   &DigitalInput_Green,
//   &DigitalInput_Red,
//   &DigitalInput_Yellow,
//   &DigitalInput_Blue,
//   &DigitalInput_Orange,
//   &DigitalInput_Start,
//   &DigitalInput_Select,
//   &DigitalInput_Tilt,
//   &DigitalInput_FlipScreen

//   // Other buttons of interest...
//   //{ PIN_07_D04_A4, "Vol+", VOLUME_INC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolUp, 0 },
//   //{ PIN_05_D02_A2, "Vol-", VOLUME_DEC_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolDown, 0 },
//   //{ PIN_06_D03_A3, "Mute", VOLUME_MUTE_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Icon, 89, 55, 16, 5, Icon_VolMute, 0 },
//   //{ PIN_D12, "Menu", MENU_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
//   //{ PIN_D13, "Home", HOME_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 },
//   //{ PIN_A5, "Back", BACK_BUTTON, -1, &BleGamepad::pressSpecialButton, &BleGamepad::releaseSpecialButton, 0, RenderInput_Text, 89, 55, 16, 5, 0, 0 }
// };

// ==========
// Hat inputs

// Assumes hats are used Hat1 -> 4

// Hat states, for all possible hats
// (for simplicity, all hats are passed to bleGamepad library, even if not used)
// To define a hat without a pin/control point, set pin to NONE - e.g. might only want to use up/down but not left/right

unsigned char HatValues[] = {0, 0, 0, 0};

// Hat used for up/down strum bar
HatInput Hat0 =
    {
        .Pins = {HAT1_Up_PIN, NONE, HAT1_Down_PIN, NONE}, .Label = "Hat0", .BluetoothHat = 0, .DefaultValue = 0, .RenderOperation = RenderInput_Hat, .XPos = -4, .YPos = 25, .RenderWidth = 15, .RenderHeight = 15, .StartIcon = Icon_DPad_Neutral,
        .ExtraOperation = {
            NONE,            // Centred
            Menus::MoveUp,   // Up
            NONE,            // Up Right
            NONE,            // Right
            NONE,            // Down Right
            Menus::MoveDown, // Down
            NONE,            // Down Left
            NONE,            // Left
            NONE             // Up Left
        },
        .CustomOperation = Custom_RenderHatStrumState,
        .Statistics = {
            NONE,           // Centred
            &Stats_HatUp,   // Up
            NONE,           // Up Right
            NONE,           // Right
            NONE,           // Down Right
            &Stats_HatDown, // Down
            NONE,           // Down Left
            NONE,           // Left
            NONE            // Up Left
        },
        .OnboardLED = {{}, {CRGB::Red, true}, {}, {}, {}, {CRGB::Green, true}, {}, {}, {}},
        .LEDConfigs = {nullptr,
                       nullptr, // new ExternalLEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Blue, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &HatEffects::MoveRainbow, .Rate = 255.0 * 2 }, //Throb, .Rate = 255.0 * 2 },
                       nullptr, nullptr, nullptr,

                       nullptr, // new ExternalLEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &HatEffects::MoveRainbow, .Rate = 27.0 }, //TimeHue, .Rate = 27.0 },
                       // new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_SimpleSet, .Rate = 27.0 },
                       // new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_Throb, .Rate = 27.0 },
                       // new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_MoveRainbow, .Rate = 27.0 },
                       // new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_Pulse, .Rate = 27.0 },
                       // new LEDConfig{.LEDNumber = LED_Hat_1,  .PrimaryColour = LED(CRGB::Red, true ), .SecondaryColour = LED(CRGB::Black, false ), .Effect = &LEDConfig::DigitalEffect_TimeHue, .Rate = 27.0 },

                       nullptr, nullptr, nullptr}};

HatInput *HatInputs[] = {
    &Hat0};

// Miscellaneous LED effects
ExternalLEDConfig *MiscLEDEffects[] = {};

ExternalLEDConfig *IdleLEDEffects[] = {
    new ExternalLEDConfig {
        // All LED's except status clone
        .LEDNumbers = { 1,2,3,4,5,6,7,8,9,
                        10,11 },
        .Effect = &GeneralArrayEffects::Random,
        .Rate =  1.5,
        .Chance = (uint32_t)(0.1 * 0xFFFF),
        .CustomTag = 64.0
     }
};

// -----------------------------------------------------
// Array sizes

int ControllerGfx_RunCount = sizeof(ControllerGfx) / sizeof(ControllerGfx[0]);
int DigitalInputs_ConfigMenu_Count = sizeof(DigitalInputs_ConfigMenu) / sizeof(DigitalInputs_ConfigMenu[0]);
int DigitalInputs_Count = sizeof(DigitalInputs) / sizeof(DigitalInputs[0]);
int AnalogInputs_Count = sizeof(AnalogInputs) / sizeof(AnalogInputs[0]);
int HatInputs_Count = sizeof(HatInputs) / sizeof(HatInputs[0]);
int MiscLEDEffects_Count = sizeof(MiscLEDEffects) / sizeof(MiscLEDEffects[0]);
int IdleLEDEffects_Count = sizeof(IdleLEDEffects) / sizeof(IdleLEDEffects[0]);

// Special case code specific to this controller

// HAT has secondary rendering, with up/down also mapped to strum bar up/down which we want to visualise
void Custom_RenderHatStrumState(HatInput *hatInput)
{
    // Special Case drawing of extra HAT interaction - the digital d-pad up/down also map to the strum bar up/down
    Display.fillRect(26, 25, 15, 15, C_BLACK);
    char c;
    if (hatInput->ValueState.Value == HAT_POS_UP)
        c = Icon_Guitar2_CenterTop;
    else if (hatInput->ValueState.Value == HAT_POS_DOWN)
        c = Icon_Guitar2_CenterBottom;
    else
        c = Icon_Guitar2_CenterOff;

    RREIcon.drawChar(26, 25, c);
}