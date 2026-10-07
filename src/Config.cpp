#include "Config.h"
#include "ConfigManager.h"
#include <Screen.h>

BoolConfig Config_InputSerialLiveOutput {
    .Type = ConfigType::Bool,
    .Metadata = {
        .Group = "Debug",
        .Label = "Live Serial Output",
        .Description = "Enables detailed serial output of input states, including digital, analog, virtual and battery states.",
        .Info = "",
        .RenderAs = ConfigRenderAs::Toggle,
        .FunctionOnSet = nullptr
    },
    .Value = DEFAULT_INPUT_SERIAL_LIVE_OUTPUT,
    .DefaultValue = false
};

void ConfigInputSerialLiveOutput_ToggleState()
{
    Config_InputSerialLiveOutput.Value = !Config_InputSerialLiveOutput.Value; 
}

IntConfig Config_InputSerialLiveOutputThrottle {
    .Type = ConfigType::Int,
    .Metadata = {
        .Group = "Debug",
        .Label = "Live Serial Output Throttle",
        .Description = "Introduces a delay when viewing live serial output of input states. Will slow down device.",
        .Info = "",
        .Min = 0,
        .Max = 1000,
        .uiMin = 0,
        .uiMax = 1000,
        .uiStep = 10,
        .SaveInPrefs = true,
        .FunctionOnSet = nullptr,
    },
    .Value = DEFAULT_INPUT_SERIAL_LIVE_OUTPUT_THROTTLE,
    .DefaultValue = 0
};

void ConfigInputSerialLiveOutputThrottle_Slow()
{
    Config_InputSerialLiveOutputThrottle.Value = 512;
    Config_InputSerialLiveOutput.Value = true;
}

void ConfigInputSerialLiveOutputThrottle_Medium()
{
    Config_InputSerialLiveOutputThrottle.Value = 256; 
    Config_InputSerialLiveOutput.Value = true;
}

void ConfigInputSerialLiveOutputThrottle_Fast()
{
    Config_InputSerialLiveOutputThrottle.Value = 64;
    Config_InputSerialLiveOutput.Value = true;
}

void ConfigInputSerialLiveOutputThrottle_Max()
{
    Config_InputSerialLiveOutputThrottle.Value = 0;
    Config_InputSerialLiveOutput.Value = true;
}

BoolConfig Config_ForceFPSDisplay {
    .Type = ConfigType::Bool,
    .Metadata = {
        .Group = "Screen",
        .Label = "Force FPS Display",
        .Description = "Forces the display of FPS in the top right corner of the screen",
        .Info = "",
        .RenderAs = ConfigRenderAs::Toggle,
        .SaveInPrefs = true,
        .FunctionOnSet = nullptr
    },
    .Value = DEFAULT_FORCE_FPS_DISPLAY,
    .DefaultValue = false
};

void ConfigForceFPSDisplay_ToggleState()
{
    Config_ForceFPSDisplay.Value = !Config_ForceFPSDisplay.Value;
};

BoolConfig Config_ScreenWhite {
    .Type = ConfigType::Bool,
    .Metadata = {
        .Group = "Screen",
        .Label = "White Screen",
        .Description = "Screen will show a solid white, handy when physically aligning panel in device where visible edges are visible",
        .Info = "",
        .FunctionOnSet = nullptr
    },
    .Value = DEFAULT_WHITE_SCREEN,
    .DefaultValue = false
};

void ConfigScreenWhite_ToggleState()
{
    // Just in case we are turning off, we blank the screen to remove any previous white (helps stop idle effect overloading too)
    Display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C_BLACK);
    Config_ScreenWhite.Value = !Config_ScreenWhite.Value;
}

// Config variables we want exposing to web/preferences
// Put in order you want things processed
BaseConfig* ConfigManager_Web_GlobalDefinitions[] = {
    reinterpret_cast<BaseConfig*>(&Config_InputSerialLiveOutput),
    reinterpret_cast<BaseConfig*>(&Config_ScreenWhite),
    reinterpret_cast<BaseConfig*>(&Config_ForceFPSDisplay),
    reinterpret_cast<BaseConfig*>(&Config_InputSerialLiveOutput),
    reinterpret_cast<BaseConfig*>(&Config_InputSerialLiveOutputThrottle)
};

int ConfigManager_Web_GlobalDefinitions_Count = sizeof(ConfigManager_Web_GlobalDefinitions) / sizeof(BaseConfig*);