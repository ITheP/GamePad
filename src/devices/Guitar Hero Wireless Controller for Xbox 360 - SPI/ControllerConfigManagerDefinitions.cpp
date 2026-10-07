// Configurable run time variables for controller, which may also be exposed into the web interface, serial interaction, etc. and
// and automated within the device for loading from/saving to Prefs

#include "LED.h"
#include "ControllerConfig.h"
#include "SerialDebug.h"
#include <Icons.h>
#include <Screen.h>

IntConfig Config_GfxGuitarModel {
    .Type = ConfigType::Int,
    .Metadata = {
        .Group = "Gfx",
        .Label = "Guitar Model",
        .Description = "Visual model used for guitar.",
        .Info = "",
        .Unit = "Guitar number",
        .Min = 0,
        .Max = 5,
        .uiMin = 0,
        .uiMax = 5,
        .SaveInPrefs = true,
        .FunctionOnSet = nullptr
    },
    .Value = 1,
    .DefaultValue = 0
};

void Config_GfxGuitarModel_CycleValue()
{
    int min = (int)Config_GfxGuitarModel.Metadata.Min;
    int max = (int)Config_GfxGuitarModel.Metadata.Max;
    int val = (int)Config_GfxGuitarModel.Value;

    val = (val >= max) ? min : (val + 1);

    Config_GfxGuitarModel.Value = val;

    Serial.println("Cycled Guitar Model to " + String(val));
    
    Display.clearDisplay();

    if (ControllerGfx_RunCount > 0)
        RenderIconRuns(ControllerGfx, ControllerGfx_RunCount, ControllerGfxOffsets[Config_GfxGuitarModel.Value], RREControllerIcons);
}

FloatConfig Config_Idle_LED_Timeout {
    .Type = ConfigType::Float,
    .Metadata = {
        .Group = "Idle",
        .Label = "LED Timeout",
        .Description = "Seconds before LED's go into idle mode.",
        .Info = "",
        .Unit = "Sec",
        .Min = 0,
        .Max = 60 * 60 * 24,
        .uiMin = 0,
        .uiMax = 60 * 10,
        .uiStep = 10,
        .FunctionOnSet = nullptr
    },
    .Value = DEFAULT_IDLE_LED_TIMEOUT,
    .DefaultValue = 0
};

FloatConfig Config_Idle_Screen_Timeout {
    .Type = ConfigType::Float,
    .Metadata = {
        .Group = "Idle",
        .Label = "Screen Timeout",
        .Description = "Seconds before screen go into idle mode.",
        .Info = "",
        .Unit = "Sec",
        .Min = 1,
        .Max = 60 * 60 * 24,
        .uiMin = 1,
        .uiMax = 60 * 10,
        .uiStep = 10,
        .FunctionOnSet = nullptr
    },
    .Value = DEFAULT_IDLE_SCREEN_TIMEOUT,
    .DefaultValue = 0
};

FloatConfig Config_Idle_Effect_Restart {
    .Type = ConfigType::Float,
    .Metadata = {
        .Group = "Idle",
        .Label = "Screen Restart",
        .Description = "Seconds before screen idle effect restarts.",
        .Info = "",
        .Unit = "Sec",
        .Min = 0,
        .Max = 60 * 60 * 24,
        .uiMin = 0,
        .uiMax = 60 * 10,
        .uiStep = 10,
        .FunctionOnSet = nullptr
    },
    .Value = DEFAULT_IDLE_EFFECT_RESTART,
    .DefaultValue = 0
};

IntConfig Config_LED_Brightness {
    .Type = ConfigType::Int,
    .Metadata = {
        .Group = "LED",
        .Label = "Brightness",
        .Description = "Global brightness of LED's",
        .Info = "Very low brightness levels may result in funny looking LED colours or fades as there isn't the resolution of brightness levels to represent subtle differences in colour",
        .Unit = "Thingies",
        .Min = 0,
        .Max = 255,
        .uiMin = 0,
        .uiMax = 255,
        .uiStep = 1,
        .FunctionOnSet = &OnGlobalLEDBrightnessConfigChange
    },
    .Value = DEFAULT_LED_BRIGHTNESS,
    .DefaultValue = 0
};

// Config variables we want exposing to web/preferences
// Put in order you want things processed
BaseConfig* ConfigManager_Web_ControllerDefinitions[] = {
    reinterpret_cast<BaseConfig*>(&Config_GfxGuitarModel),
    reinterpret_cast<BaseConfig*>(&Config_InputSerialLiveOutput),
    reinterpret_cast<BaseConfig*>(&Config_Idle_LED_Timeout),
    reinterpret_cast<BaseConfig*>(&Config_Idle_Screen_Timeout),
    reinterpret_cast<BaseConfig*>(&Config_Idle_Effect_Restart),
    reinterpret_cast<BaseConfig*>(&Config_LED_Brightness)
};

int ConfigManager_Web_ControllerDefinitions_Count = sizeof(ConfigManager_Web_ControllerDefinitions) / sizeof(BaseConfig*);

SerialDebugFunction ConfigManager_SerialDebugFunctions[] = {    
    {Description : "Gfx"}, // Heading

    {"Change Guitar",
     "Cycles through controller gfx used",
     "",
     &Config_GfxGuitarModel_CycleValue}
};

int ConfigManager_SerialDebugFunctions_Count = sizeof(ConfigManager_SerialDebugFunctions) / sizeof(SerialDebugFunction);