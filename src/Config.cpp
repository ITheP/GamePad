#include "Config.h"
#include "ConfigManager.h"
#include <Screen.h>
#include <GamePad.h>

IntConfig Config_StartupThrottle {
    //.Type = ConfigType::Int,
    .Metadata = {
        .Group = "Startup",
        .Label = "Startup Throttle",
        .Description = "Small delays (in ms) at points in startup sequence.",
        .Info = "Not required, simply there to give some weight to the startup UX plus a chance to see the UI more clearly.",
        .RenderAs = ConfigRenderAs::Toggle,
        .Unit = "ms",
        .Min = 0,
        .Max = 1000,
        .uiMin = 0,
        .uiMax = 1000,
        .uiStep = 10,
        .PrefsId = "Start.Throttle",
        .FunctionOnSet = nullptr,
    },
    .Value = DEFAULT_STARTUP_THROTTLE,
    .DefaultValue = DEFAULT_STARTUP_THROTTLE
};

BoolConfig Config_InputSerialLiveOutput {
    //.Type = ConfigType::Bool,
    .Metadata = {
        .Group = "Debug",
        .Label = "Live Serial Output",
        .Description = "Enables detailed serial output of input states, including digital, analog, virtual and battery states.",
        .Info = "",
        .RenderAs = ConfigRenderAs::Toggle,
        .FunctionOnSet = nullptr
    },
    .Value = DEFAULT_INPUT_SERIAL_LIVE_OUTPUT,
    .DefaultValue = DEFAULT_INPUT_SERIAL_LIVE_OUTPUT
};

void ConfigInputSerialLiveOutput_ToggleState()
{
    Config_InputSerialLiveOutput.Value = !Config_InputSerialLiveOutput.Value; 
}

IntConfig Config_InputSerialLiveOutputThrottle {
    //.Type = ConfigType::Int,
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
    .DefaultValue = DEFAULT_INPUT_SERIAL_LIVE_OUTPUT_THROTTLE
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
    //.Type = ConfigType::Bool,
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
    .DefaultValue = DEFAULT_FORCE_FPS_DISPLAY
};

void ConfigForceFPSDisplay_ToggleState()
{
    Config_ForceFPSDisplay.Value = !Config_ForceFPSDisplay.Value;
};

// Extra function pointer here to make when screen no longer white it's redrawn accordingly
BoolConfig Config_ScreenWhite {
    //.Type = ConfigType::Bool,
    .Metadata = {
        .Group = "Screen",
        .Label = "White Screen",
        .Description = "Screen will show a solid white, handy when physically aligning panel in device where visible edges are visible",
        .Info = "",
        .FunctionOnSet = &OnWhiteConfigChange
    },
    .Value = DEFAULT_WHITE_SCREEN,
    .DefaultValue = DEFAULT_WHITE_SCREEN
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
    reinterpret_cast<BaseConfig*>(&Config_StartupThrottle),
    reinterpret_cast<BaseConfig*>(&Config_ScreenWhite),
    reinterpret_cast<BaseConfig*>(&Config_ForceFPSDisplay),
    reinterpret_cast<BaseConfig*>(&Config_InputSerialLiveOutput),
    reinterpret_cast<BaseConfig*>(&Config_InputSerialLiveOutputThrottle)
};

int ConfigManager_Web_GlobalDefinitions_Count = sizeof(ConfigManager_Web_GlobalDefinitions) / sizeof(BaseConfig*);

IntConfig Config_Hardware_DebounceDelay {
    //.Type = ConfigType::Int,
    .Metadata = {
        .Group = "Hardware",
        .Label = "Debounce Delay",
        .Description = "Debounce delay (in microseconds) on digital inputs.",
        .Info = "Debounce delay is where no digital on/off is reprocessed within this time span upon a change, to help with electrical noise when a change takes place. Smaller numbers = lower latency but more chance of bad readings. 10ms (10000µs) is a decent figure to use.",
        .RenderAs = ConfigRenderAs::Toggle,
        .Unit = "µs",
        .Min = 0,
        .Max = 20000,
        .uiMin = 2000,
        .uiMax = 20000,
        .uiStep = 250,
        .PrefsId = "HW.Debounce",
        .FunctionOnSet = nullptr,
    },
    .Value = DEFAULT_DEBOUNCE_DELAY,
    .DefaultValue = DEFAULT_DEBOUNCE_DELAY
};

// Config variables we want exposing to web/preferences
// Put in order you want things processed
BaseConfig* ConfigManager_Web_HardwareDefinitions[] = {
    reinterpret_cast<BaseConfig*>(&Config_Hardware_DebounceDelay)
};

int ConfigManager_Web_HardwareDefinitions_Count = sizeof(ConfigManager_Web_HardwareDefinitions) / sizeof(BaseConfig*);
