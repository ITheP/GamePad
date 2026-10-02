#pragma once

#pragma once
#include <string>
#include <cstring>
#include <cstdint>

// ------------------------------------------------------------
// ENUMS
// ------------------------------------------------------------

enum ConfigType {
    Int,
    Bool,
    Float,
    CharStar,
    Colour
};

static const char* ConfigTypeDescriptions[] = {
    "Int",
    "Bool",
    "Float",
    "CharStar",
    "Colour"
};

enum class ConfigManagerUpdateResult {
    OK,
    NumberTooLow,
    NumberTooHigh,
    StringTooShort,
    StringTooLong,
    StringMissing,
    ColourOutOfRange,
    UnknownConfigType
};

static const char* ConfigManagerUpdateResultDescriptions[] = {
    "OK",
    "Number Too Low",
    "Number Too High",
    "String Too Short",
    "String Too Long",
    "String Missing",
    "Colour Out Of Range",
    "Unknown Config Type"
};

// Forward declaration
struct BaseConfig;

// Callback signature
typedef ConfigManagerUpdateResult (*OnSetCallback)(BaseConfig*, void*);

// ------------------------------------------------------------
// Base/common config variables
// ------------------------------------------------------------

struct ConfigMetadata {  
    std::string Group;  
    std::string Label;
    std::string Description;
    std::string Info;
    float min;
    float max;
    float uiMin;
    float uiMax;
    float uiStep;

    OnSetCallback FunctionOnSet;
};

// ------------------------------------------------------------
// POD variables
// ------------------------------------------------------------

// No value etc. for base, just a structure we can cast to to work out type etc.
struct BaseConfig {
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
};

struct IntConfig {
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    int Value;
    int DefaultValue;
};

struct BoolConfig {
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    bool Value;
    bool DefaultValue;
};

struct StringConfig {
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    char* Value;
    char* DefaultValue;
};

struct FloatConfig {
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    float Value;
    float DefaultValue;
};

struct ColorConfig {
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    uint8_t r, g, b;
    uint8_t defaultR, defaultG, defaultB;
};

struct CheckListItem {
    bool Value;
    bool DefaultValue;
    std::string Label;
};

struct CheckListConfig {
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    CheckListItem CheckList[];

};

// ------------------------------------------------------------
// Config Manager itself
// ------------------------------------------------------------

class ConfigManager {
public:
    static int ConfigId;
    static BaseConfig **ConfigMap;
    static int ConfigCount;
    static int ConfigCapacity;


    //static void AddConfig(BaseConfig* config);
    static BaseConfig* GetConfig(int id);
    static ConfigManagerUpdateResult UpdateConfigById(int id, void* value);
    static void AddConfigArray(BaseConfig** configs, int count);

    template <typename T>
    static void AddConfig(T *config)
    {
        EnsureCapacity();

        Serial.println("Adding config [" + String(config->Id) + "." + String(ConfigTypeDescriptions[config->Type]) + "]: " + String(config->Metadata.Label.c_str()));

        // All your config structs begin with Type, Id, Metadata
        BaseConfig *base = reinterpret_cast<BaseConfig *>(config);

        base->Id = ConfigCount;
        ConfigMap[ConfigCount++] = base;

        // Check if is also saved in Preferences and load in value if it is
    }


private:
    static void EnsureCapacity();
};

