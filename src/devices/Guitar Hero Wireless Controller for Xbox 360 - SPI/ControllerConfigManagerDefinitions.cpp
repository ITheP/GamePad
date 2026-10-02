#include "LED.h"
#include "ControllerConfig.h"

FloatConfig Config_Idle_LED_Timeout {
    .Type = ConfigType::Float,
    .Id = 0,
    .Metadata = {
        .Group = "Idle",
        .Label = "LED Timeout",
        .Description = "Seconds before LED's go into idle mode.",
        .Info = "",
        .min = 0,
        .max = 60 * 60 * 24,
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
    .Id = 0,
    .Metadata = {
        .Group = "Idle",
        .Label = "Screen Timeout",
        .Description = "Seconds before screen go into idle mode.",
        .Info = "",
        .min = 0,
        .max = 60 * 60 * 24,
        .uiMin = 0,
        .uiMax = 60 * 10,
        .uiStep = 10,
        .FunctionOnSet = nullptr
    },
    .Value = DEFAULT_IDLE_SCREEN_TIMEOUT,
    .DefaultValue = 0
};

FloatConfig Config_Idle_Effect_Restart {
    .Type = ConfigType::Float,
    .Id = 0,
    .Metadata = {
        .Group = "Idle",
        .Label = "Screen Restart",
        .Description = "Seconds before screen idle effect restarts.",
        .Info = "",
        .min = 0,
        .max = 60 * 60 * 24,
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
    .Id = 0,
    .Metadata = {
        .Group = "LED",
        .Label = "Brightness",
        .Description = "Global maximum brightness of LED's",
        .Info = "Very low brightness levels may result in funny looking LED colours or fades as there isn't the resolution of brightness levels to represent subtle differences in colour",
        .min = 0,
        .max = 255,
        .uiMin = 0,
        .uiMax = 60 * 10,
        .uiStep = 10,
        .FunctionOnSet = &OnGlobalLEDBrightnessConfigChange
    },
    .Value = DEFAULT_LED_BRIGHTNESS,
    .DefaultValue = 0
};

// Config variables we want exposing to web/preferences
// Put in order you want things processed
BaseConfig* ConfigManager_ControllerDefinitions[] = {
    reinterpret_cast<BaseConfig*>(&Config_Idle_LED_Timeout),
    reinterpret_cast<BaseConfig*>(&Config_Idle_Screen_Timeout),
    reinterpret_cast<BaseConfig*>(&Config_Idle_Effect_Restart),
    reinterpret_cast<BaseConfig*>(&Config_LED_Brightness)
};

int ConfigManager_ControllerDefinitions_Size = sizeof(ConfigManager_ControllerDefinitions) / sizeof(BaseConfig*);