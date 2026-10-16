# Config Example...

ConfigManager Definition....

``` c++
IntConfig Config_LED_Brightness {
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
        .FunctionOnSet = &GlobalLEDBrightnessConfigChange
    },
    .Value = DEFAULT_LED_BRIGHTNESS,
    .DefaultValue = 0
};
```

GlobalLEDBrightnessConfigChange is a function that will be passed the Config, the Value in a sanitised format (e.g. bound to Min/Max range) and it is the responsibility of .FunctionOnSet to save this to the Config->Value. This allows the function to do additional validation and manipulation.

``` c++
BaseConfig* ConfigManager_Web_ControllerDefinitions[] = {
    reinterpret_cast<BaseConfig*>(&Config_LED_Brightness)
    etc.
};
```

List of Config picked up and used by Web Service (automated Config front end)

SerialDebug.h

Command with no parameters...

``` c++
    {"White Screen",
     "Toggle on/off the display to show a completely white screen",
     "Switches the display to a completely white screen, useful for physical alignment when fitting the screen panel into a device - you can see where the visible edges are.",
     &ConfigScreenWhite_ToggleState},
```

...ConfigScreenWhite_ToggleState is a parameterless function

Command with parameters

``` c++
    {"Set Brightness",
     "Global LED brightness",
     "Sets LED brightness to the given integer, 0-255.\n  Example: Set Brightness 200",
     nullptr,
     SerialDebugFunctionParameterType::Int,
     &SerialDebugTrampoline<int, ApplyGlobalLEDBrightnessToConfig>},
```

...ApplyGlobalLEDBrightnessToConfig has an int passed to it


Example handling the above...

GlobalLEDBrightnessConfigChange is called from the ConfigManager when the Web site sends a change over to the device, specifically UpdateConfig(…)

UpdateConfig etc. is responsible for processing data from the web, converting to relevant types, updating Config->Value and saving to Prefs if required. If a FunctionOnSet is set then Config->Value is NOT updated. The function this calls is responsible. This allows for that function to do additional validation, manipulation, etc.

e.g.

``` c++
ConfigManagerUpdateResult GlobalLEDBrightnessConfigChange(BaseConfig *config, void *brightness)
{
  IntConfig *c = (IntConfig *)config;
  int b = *static_cast<int *>(brightness);

  c->Value = b;
  ApplyGlobalLEDBrightnessFromConfig();

  return ConfigManagerUpdateResult::OK;
}
```

Note that no saving of c->Value takes place in there etc. - that is handled by UpdateConfig(…)

``` c++
void ApplyGlobalLEDBrightnessFromConfig()
{
  FastLED.setBrightness(Config_LED_Brightness.Value);
}
```

For the SerialDebug handling, this has slightly more code to it.

``` c++
void ApplyGlobalLEDBrightnessToConfig(int brightness)
{
  IntConfig *config = &Config_LED_Brightness;

  auto result = ConfigManager::UpdateConfig((BaseConfig *)config, &brightness);
  
  if (result == ConfigManagerUpdateResult::OK)
  {
    Serial_INFO;
    Serial.printf("Set Global LED Brightness to %d", brightness);
  }
  else
  {
    Serial_ERROR;
    Serial.printf("Unable to set Global LED Brightness to %d: %s. Valid number range = %d->%d.\n",
                  brightness,
                  ConfigManagerUpdateResultDescriptions[(int)result],
                  (int)(config->Metadata.Min),
                  (int)(config->Metadata.Max));
  }
}
```

It passes on the config and value to UpdateConfig (which in turn handles it as if it came in via a web request, calls the Configs .FunctionOnSet, saves values etc.

We can then process the result of that accordingly e.g. for serial output feedback.

