#include "Config.h"
#include "ConfigManager.h"
#include <Screen.h>
#include <GamePad.h>

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
        .PrefsId = "Debug.LSOT",
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
        .PrefsId = "Screen.ForceFPS",
        .FunctionOnSet = nullptr
    },
    .Value = DEFAULT_FORCE_FPS_DISPLAY,
    .DefaultValue = false
};

void ConfigForceFPSDisplay_ToggleState()
{
    Config_ForceFPSDisplay.Value = !Config_ForceFPSDisplay.Value;
};

// Extra function pointer here to make when screen no longer white it's redrawn accordingly
BoolConfig Config_ScreenWhite {
    .Type = ConfigType::Bool,
    .Metadata = {
        .Group = "Screen",
        .Label = "White Screen",
        .Description = "Screen will show a solid white, handy when physically aligning panel in device where visible edges are visible",
        .Info = "",
        .FunctionOnSet = &OnWhiteConfigChange
    },
    .Value = DEFAULT_WHITE_SCREEN,
    .DefaultValue = false
};

ConfigManagerUpdateResult OnWhiteConfigChange(BaseConfig* cfg, void* newValue)
{
    BoolConfig* c = (BoolConfig*)cfg;
    int v = *static_cast<bool*>(newValue);

    c->Value = v;

    if (!v)
        DrawMainScreen();
    
    return ConfigManagerUpdateResult::OK;
}

void ConfigScreenWhite_ToggleState()
{
    Config_ScreenWhite.Value = !Config_ScreenWhite.Value;

    // if we are turning off - need to redisplay the main screen
    if (!Config_ScreenWhite.Value)
        DrawMainScreen();
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