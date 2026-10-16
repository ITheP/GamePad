#pragma once

//#include "ConfigManager.h"
#include <SerialDebug.h>

extern IntConfig Config_GfxGuitarModel;
void Config_GfxGuitarModel_CycleValue();
ConfigManagerUpdateResult OnGfxGuitarModelConfigChange(BaseConfig* cfg, void* newValue);

extern FloatConfig Config_Idle_LED_Timeout;

extern FloatConfig Config_Idle_Screen_Timeout;

extern FloatConfig Config_Idle_Effect_Restart;

extern IntConfig Config_LED_Brightness;

// Config variables we want exposing to web/preferences
// Put in order you want things processed
extern BaseConfig* ConfigManager_Web_ControllerDefinitions[];
extern int ConfigManager_Web_ControllerDefinitions_Count;

extern SerialDebugFunction ConfigManager_SerialDebugFunctions[];
extern int ConfigManager_SerialDebugFunctions_Count;