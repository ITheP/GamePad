#pragma once

extern FloatConfig Config_Idle_LED_Timeout;

extern FloatConfig Config_Idle_Screen_Timeout;

extern FloatConfig Config_Idle_Effect_Restart;

extern IntConfig Config_LED_Brightness;

// Config variables we want exposing to web/preferences
// Put in order you want things processed
extern BaseConfig* ConfigManager_ControllerDefinitions[];
extern int ConfigManager_ControllerDefinitions_Size;