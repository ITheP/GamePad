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

// int ConfigManager::ConfigId = 0;
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
    {
        memcpy(newMap, ConfigMap, sizeof(BaseConfig *) * ConfigCount);
        free(ConfigMap);
    }

    ConfigMap = newMap;
    ConfigCapacity = newCapacity;
}

void ConfigManager::AddConfigArray(BaseConfig **configs, int count)
{
    for (size_t i = 0; i < count; i++)
    {
        AddConfig(configs[i]);

        // Serial.printf("ConfigCount: %d, ConfigCapacity: %d\n", ConfigCount, ConfigCapacity);
        // Serial.printf("Config [%3d.%-12s]: %s\n", configs[i]->Id, ConfigTypeDescriptions[(int)configs[i]->Type], configs[i]->Metadata.Label.c_str());

        // Serial.println("Config added");
    }
}

BaseConfig *ConfigManager::GetConfig(int id)
{
    return ConfigMap[id];
}

// Renders json equivalent to serial so we can view it e.g. for checking
void ConfigManager::RenderConfigJsonToSerial()
{
    std::ostringstream json = GetConfigAsJson();

    Serial.println(json.str().c_str());
}

void ConfigManager::RenderConfigToSerial()
{
    std::ostringstream output;
    output << "Config...\n";

    for (int i = 0; i < ConfigCount; i++)
    {
        auto config = ConfigMap[i];

        if (i > 0)
            output << "\n";

        output << "[" << config->Id << "] "
             << config->Metadata.Group << "." << config->Metadata.Label << "\n"
             << "Type: " << ConfigTypeDescriptions[(int)config->Type] << ", ";

        switch (config->Type)
        {
        case ConfigType::Int:
            output << "Value: " << reinterpret_cast<IntConfig *>(config)->Value << ", "
                 << "DefaultValue: " << reinterpret_cast<IntConfig *>(config)->DefaultValue;
            break;

        case ConfigType::Float:
            output << "Value: " << reinterpret_cast<FloatConfig *>(config)->Value << ", "
                 << "DefaultValue: " << reinterpret_cast<FloatConfig *>(config)->DefaultValue;
            break;
        case ConfigType::Bool:
            output << "Value: " << reinterpret_cast<BoolConfig *>(config)->Value << ", "
                 << "DefaultValue: " << reinterpret_cast<BoolConfig *>(config)->DefaultValue;
            break;
        case ConfigType::String:
            output << "Value: " << reinterpret_cast<StringConfig *>(config)->Value << ","
                 << "DefaultValue: " << reinterpret_cast<StringConfig *>(config)->DefaultValue;
            break;
        case ConfigType::Colour:
            output << "Value: {"
                 << "r: " << (int)reinterpret_cast<ColorConfig *>(config)->r << ","
                 << "g: " << (int)reinterpret_cast<ColorConfig *>(config)->g << ","
                 << "b: " << (int)reinterpret_cast<ColorConfig *>(config)->b << "},"
                 << "DefaultValue: {"
                 << "r: " << (int)reinterpret_cast<ColorConfig *>(config)->defaultR << ","
                 << "g: " << (int)reinterpret_cast<ColorConfig *>(config)->defaultG << ","
                 << "b: " << (int)reinterpret_cast<ColorConfig *>(config)->defaultB << "}";
            break;
        }

        output << "\nMetadata...\n"
             << "\tDescription: " << config->Metadata.Description << "\n"
             << "\tInfo: " << config->Metadata.Info << "\n"
             << "\tRenderAs: " << ConfigRenderAsDescriptions[(int)config->Metadata.RenderAs] << ", "
             << "SaveInPrefs: " << config->Metadata.SaveInPrefs << ", "
             << "Unit: " << config->Metadata.Unit << "\n"
             << "\tMin: " << config->Metadata.Min << ", "
             << "Max: " << config->Metadata.Max << ", "
             << "uiMin: " << config->Metadata.uiMin << ", "
             << "uiMax: " << config->Metadata.uiMax << ", "
             << "uiStep: " << config->Metadata.uiStep << ", "
             << "Image: " << config->Metadata.Image << ", "
             << "ImageVariants: " << config->Metadata.ImageVariants << ", "
             << "ImageSplitVertically: " << ( config->Metadata.ImageSplitVertically ? "true" : "false");
    }

    output << "\n";

    Serial.print(output.str().c_str());
}

std::ostringstream ConfigManager::GetConfigAsJson()
{
    std::ostringstream json;
    json << "{\"config\": [\n";

    for (int i = 0; i < ConfigCount; i++)
    {
        auto config = ConfigMap[i];

        if (i > 0)
            json << ",\n";

        // TODO: Only push out json with values, ignore otherwise - save bandwidth!
        json << "{\"Id\": " << config->Id << ","
             << "\"Type\": \"" << ConfigTypeDescriptions[(int)config->Type] << "\","
             << "\"Metadata\": {"
             << "\"Group\": \"" << config->Metadata.Group << "\","
             << "\"Label\": \"" << config->Metadata.Label << "\","
             << "\"Description\": \"" << config->Metadata.Description << "\","
             << "\"Info\": \"" << config->Metadata.Info << "\","
             << "\"RenderAs\": \"" << ConfigRenderAsDescriptions[(int)config->Metadata.RenderAs] << "\","
             << "\"SaveInPrefs\": " << config->Metadata.SaveInPrefs << ","
             << "\"Unit\": \"" << config->Metadata.Unit << "\","
             << "\"Min\": " << config->Metadata.Min << ","
             << "\"Max\": " << config->Metadata.Max << ","
             << "\"uiMin\": " << config->Metadata.uiMin << ","
             << "\"uiMax\": " << config->Metadata.uiMax << ","
             << "\"uiStep\": " << config->Metadata.uiStep << ",";

             if (config->Metadata.ImageVariants > 0)
                json << "\"Image\": \"" << config->Metadata.Image << "\", ";

             if (config->Metadata.ImageVariants > 0)
                json << "\"ImageVariants\": " << config->Metadata.ImageVariants << ",";

             if (config->Metadata.ImageSplitVertically)
                json << "\"ImageSplitVertically\": true,";

        switch (config->Type)
        {
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

        json << "}}";
    }

    json << "\n]}";

    return json;
}


ConfigManagerUpdateResult ConfigManager::AttemptUpdateConfigById(char *id, char *value)
{
    // ---- 1. Parse the id ------------------------------------------------
    if (id == nullptr || *id == '\0') {
        return ConfigManagerUpdateResult::InvalidId;
    }

    errno = 0;
    char *idEnd = nullptr;
    long idLong = strtol(id, &idEnd, 10);

    // Ids are non-negative. 0 is valid, negatives are not.
    if (idEnd == id || *idEnd != '\0'
        || errno == ERANGE
        || idLong < 0 || idLong > INT_MAX) {
        return ConfigManagerUpdateResult::InvalidId;
    }

    int configId = (int)idLong;

    // ---- 2. Look up the config (direct index) --------------------------
    if (configId < 0 || configId >= ConfigCount) {
        return ConfigManagerUpdateResult::IdNotFound;
    }

    BaseConfig *base = ConfigMap[configId];
    if (base == nullptr) {
        return ConfigManagerUpdateResult::IdNotFound;
    }

    // ---- 3. Validate + convert the value by type ------------------------
    if (value == nullptr) {
        return ConfigManagerUpdateResult::InvalidNumber;
    }

    switch (base->Type)
    {
        case ConfigType::Int:
        {
            errno = 0;
            char *end = nullptr;
            long v = strtol(value, &end, 10);
            if (end == value || *end != '\0' || errno == ERANGE
                || v < INT_MIN || v > INT_MAX) {
                return ConfigManagerUpdateResult::InvalidNumber;
            }
            // UpdateConfig reads this as `int*` — so store into an int.
            int typedValue = (int)v;
            return UpdateConfig(base, &typedValue);
        }

        case ConfigType::Float:
        {
            errno = 0;
            char *end = nullptr;
            float v = strtof(value, &end);
            if (end == value || *end != '\0' || errno == ERANGE) {
                return ConfigManagerUpdateResult::InvalidNumber;
            }
            return UpdateConfig(base, &v);
        }

        case ConfigType::Bool:
        {
            bool v;
            if      (strcmp(value, "true")  == 0 || strcmp(value, "1") == 0) v = true;
            else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) v = false;
            else return ConfigManagerUpdateResult::InvalidBool;
            return UpdateConfig(base, &v);
        }

        case ConfigType::String:
        {
            // Strip the outer quotes cJSON added.
            char *raw = value;
            size_t len = strlen(value);
            if (len >= 2 && value[0] == '"' && value[len - 1] == '"') {
                value[len - 1] = '\0';
                raw = value + 1;
            }
            // char* converts to void* implicitly — no cast needed.
            return UpdateConfig(base, raw);
        }

        case ConfigType::Colour:
        {
            const char *c = value;
            if (*c == '"') ++c;

            size_t hexLen = 0;
            while (isxdigit((unsigned char)c[hexLen])) ++hexLen;
            if (hexLen != 6) return ConfigManagerUpdateResult::InvalidNumber;
            if (c[hexLen] != '\0' && !(c[hexLen] == '"' && c[hexLen + 1] == '\0')) {
                return ConfigManagerUpdateResult::InvalidNumber;
            }

            // Parse RRGGBB, then split into three bytes for UpdateConfig,
            // which reads the value as a uint8_t[3] (r, g, b).
            uint32_t rgb = (uint32_t)strtoul(c, nullptr, 16);
            uint8_t bytes[3];
            bytes[0] = (uint8_t)((rgb >> 16) & 0xFF);   // r
            bytes[1] = (uint8_t)((rgb >>  8) & 0xFF);   // g
            bytes[2] = (uint8_t)( rgb        & 0xFF);   // b

            return UpdateConfig(base, bytes);
        }
    }

    return ConfigManagerUpdateResult::UnknownConfigType;
}
ConfigManagerUpdateResult ConfigManager::UpdateConfigById(int id, void *value)
{
    BaseConfig *base = ConfigMap[id];

    return UpdateConfig(base, value);
}

ConfigManagerUpdateResult ConfigManager::UpdateConfig(BaseConfig *base, void *value)
{
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

        if (v > (int)c->Metadata.Max)
            return ConfigManagerUpdateResult::NumberTooHigh;
        if (v < (int)c->Metadata.Min)
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

        if (v > c->Metadata.Max)
            return ConfigManagerUpdateResult::NumberTooHigh;
        if (v < c->Metadata.Min)
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

        if (len < (int)c->Metadata.Min)
            return ConfigManagerUpdateResult::StringTooShort;
        if (len > (int)c->Metadata.Max)
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