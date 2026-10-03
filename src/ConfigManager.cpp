#include <variant>
#include <sstream>
#include "ConfigManager.h"

// --- Configuration System Definitions (High Performance, Compile-Time Safe) ---

// ConfigManager provides a level of automated dynamic configuration to the system, with the goal of trying to retain
// as much performance as possible!

// We aren't overly dynamic with configurable options as much is tied to physical hardware of the device - we don't want someone
// messing around with something that realistically can't be changed. If it does need changing, then the low level config needs adjustment.
// This is for things like led colours and timings, screen savers, other timings, input levels, etc. - stuff that is safe to change at run time
// Also load at boot, adjust in web interface and save back to device
// Web interface needs to auto-generate configuration options based on configurable option metadata, which is also
// used for value validation
// Wanted to retain direct reference to variables in code for max speed;

// ConfigManager only cares about variables specified for exposure to load/save, web etc. - other variables have metadata ignored
// and are just used as simple variables.
// List of cared about values is specified in
//  - Controller config
//  - Main code on boot

int ConfigManager::ConfigId = 0;
BaseConfig **ConfigManager::ConfigMap = nullptr;
int ConfigManager::ConfigCount = 0;
int ConfigManager::ConfigCapacity = 0;

void ConfigManager::EnsureCapacity()
{
    if (ConfigCount < ConfigCapacity)
        return;

    int newCapacity = (ConfigCapacity == 0) ? 16 : ConfigCapacity + 20;

    BaseConfig **newMap = (BaseConfig **)malloc(sizeof(BaseConfig *) * newCapacity);

    if (ConfigMap)
        memcpy(newMap, ConfigMap, sizeof(BaseConfig *) * ConfigCount);

    ConfigMap = newMap;
    ConfigCapacity = newCapacity;
}

void ConfigManager::AddConfigArray(BaseConfig **configs, int count)
{
    for (size_t i = 0; i < count; i++)
        AddConfig(configs[i]);
}

BaseConfig *ConfigManager::GetConfig(int id)
{
    return ConfigMap[id];
}

void ConfigManager::RenderConfigToJson(std::ostringstream& json)
{
    for (int i = 0; i < ConfigCount; i++)
    {
        auto config = ConfigMap[i];

        if (i > 0)
            json << ",";

        json << "{\"Id\": " << config->Id << ","
             << "\"Type\": \"" << ConfigTypeDescriptions[(int)config->Type] << "\","
             << "\"Metadata\": {"
             << "\"Group\": \"" << config->Metadata.Group << "\"," 
             << "\"Label\": \"" << config->Metadata.Label << "\","
             << "\"Description\": \"" << config->Metadata.Description << "\","
             << "\"Info\": \"" << config->Metadata.Info << "\","
             << "\"min\": " << config->Metadata.min << ","
             << "\"max\": " << config->Metadata.max << ","
             << "\"uiMin\": " << config->Metadata.uiMin << ","
             << "\"uiMax\": " << config->Metadata.uiMax << ","
             << "\"uiStep\": " << config->Metadata.uiStep << ",";

        switch (config->Type){
            case ConfigType::Int:
            json << "\"Value\": " << reinterpret_cast<IntConfig *>(config)->Value << ","
                 << "\"DefaultValue\": " << reinterpret_cast<IntConfig *>(config)->DefaultValue;
        break;

    case ConfigType::Float:
        json << "\"Value\": " << reinterpret_cast<FloatConfig *>(config)->Value << ","
             << "\"DefaultValue\": " << reinterpret_cast<FloatConfig *>(config)->DefaultValue;
        break;
    case ConfigType::Bool:
        json << "\"Value\": " << reinterpret_cast<BoolConfig *>(config)->Value << ","
             << "\"DefaultValue\": " << reinterpret_cast<BoolConfig *>(config)->DefaultValue;
        break;
    case ConfigType::String:
        json << "\"Value\": \"" << reinterpret_cast<StringConfig *>(config)->Value << "\","
             << "\"DefaultValue\": \"" << reinterpret_cast<StringConfig *>(config)->DefaultValue << "\"";
        break;
    case ConfigType::Colour:
        json << "\"Value\": {"
             << "\"r\": " << (int)reinterpret_cast<ColorConfig *>(config)->r << ","
             << "\"g\": " << (int)reinterpret_cast<ColorConfig *>(config)->g << ","
             << "\"b\": " << (int)reinterpret_cast<ColorConfig *>(config)->b << "},"
             << "\"DefaultValue\": {"
             << "\"r\": " << (int)reinterpret_cast<ColorConfig *>(config)->defaultR << ","
             << "\"g\": " << (int)reinterpret_cast<ColorConfig *>(config)->defaultG << ","
             << "\"b\": " << (int)reinterpret_cast<ColorConfig *>(config)->defaultB << "}";
        break;
    }

    json << "}";
}
}

ConfigManagerUpdateResult ConfigManager::UpdateConfigById(int id, void *value)
{
    BaseConfig *base = ConfigMap[id];
    auto result = ConfigManagerUpdateResult::OK;

    switch (base->Type)
    {
    // ------------------------------------------------------------
    // INT
    // ------------------------------------------------------------
    case ConfigType::Int:
    {
        IntConfig *c = (IntConfig *)base;
        int v = *static_cast<int *>(value);

        if (v > (int)c->Metadata.max)
            return ConfigManagerUpdateResult::NumberTooHigh;
        if (v < (int)c->Metadata.min)
            return ConfigManagerUpdateResult::NumberTooLow;

        // callback BEFORE storing value
        // If called, is also responsible for saving the value
        // (it may transform value before saving)
        if (c->Metadata.FunctionOnSet)
            result = c->Metadata.FunctionOnSet(base, value);
        else
            c->Value = v;

        if (result == ConfigManagerUpdateResult::OK)
        {
            // Save to preferences
        }

        return result;
    }

    // ------------------------------------------------------------
    // FLOAT
    // ------------------------------------------------------------
    case ConfigType::Float:
    {
        FloatConfig *c = (FloatConfig *)base;
        float v = *static_cast<float *>(value);

        if (v > c->Metadata.max)
            return ConfigManagerUpdateResult::NumberTooHigh;
        if (v < c->Metadata.min)
            return ConfigManagerUpdateResult::NumberTooLow;

        if (c->Metadata.FunctionOnSet)
            result = c->Metadata.FunctionOnSet(base, value);
        else
            c->Value = v;

        if (result == ConfigManagerUpdateResult::OK)
        {
            // Save to preferences
        }

        return result;
    }

    // ------------------------------------------------------------
    // BOOL
    // ------------------------------------------------------------
    case ConfigType::Bool:
    {
        BoolConfig *c = (BoolConfig *)base;
        bool v = *static_cast<bool *>(value);

        if (c->Metadata.FunctionOnSet)
            result = c->Metadata.FunctionOnSet(base, value);
        else
            c->Value = v;

        if (result == ConfigManagerUpdateResult::OK)
        {
            // Save to preferences
        }

        return result;
    }

    // ------------------------------------------------------------
    // STRING
    // ------------------------------------------------------------
    case ConfigType::String:
    {
        StringConfig *c = (StringConfig *)base;
        char *str = static_cast<char *>(value);

        int len = strlen(str);

        if (len < (int)c->Metadata.min)
            return ConfigManagerUpdateResult::StringTooShort;
        if (len > (int)c->Metadata.max)
            return ConfigManagerUpdateResult::StringTooLong;

        if (c->Metadata.FunctionOnSet)
            result = c->Metadata.FunctionOnSet(base, value);
        else
            c->Value = str;

        if (result == ConfigManagerUpdateResult::OK)
        {
            // Save to preferences
        }

        return result;
    }

    // ------------------------------------------------------------
    // COLOUR
    // ------------------------------------------------------------
    case ConfigType::Colour:
    {
        ColorConfig *c = (ColorConfig *)base;
        uint8_t *rgb = static_cast<uint8_t *>(value);

        if (rgb[0] > 255 || rgb[1] > 255 || rgb[2] > 255)
            return ConfigManagerUpdateResult::ColourOutOfRange;

        if (c->Metadata.FunctionOnSet)
            result = c->Metadata.FunctionOnSet(base, value);
        else
        {
            c->r = rgb[0];
            c->g = rgb[1];
            c->b = rgb[2];
        }

        if (result == ConfigManagerUpdateResult::OK)
        {
            // Save to preferences
        }

        return result;
    }
    }

    return ConfigManagerUpdateResult::UnknownConfigType;
}