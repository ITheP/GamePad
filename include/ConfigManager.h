#pragma once

#pragma once
#include <string>
#include <cstring>
#include <cstdint>

// ------------------------------------------------------------
// ENUMS
// ------------------------------------------------------------

enum class ConfigType
{
    Int,
    Bool,
    Float,
    String,
    Colour
};

static const char *ConfigTypeDescriptions[] = {
    "Int",
    "Bool",
    "Float",
    "String",
    "Colour"};

enum class ConfigRenderAs
{
    Default = 0,
    CheckBox,
    Toggle
};

static const char *ConfigRenderAsDescriptions[] = {
    "Default",
    "CheckBox",
    "Toggle"};

enum class ConfigManagerUpdateResult
{
    OK,
    NumberTooLow,
    NumberTooHigh,
    StringTooShort,
    StringTooLong,
    StringMissing,
    ColourOutOfRange,
    UnknownConfigType,

    IdMissing,
    InvalidId,
    IdNotFound,
    InvalidNumber,
    InvalidBool,
    InvalidString,
    InvalidColour
};

static const char *ConfigManagerUpdateResultDescriptions[] = {
    "OK",
    "Number Too Low",
    "Number Too High",
    "String Too Short",
    "String Too Long",
    "String Missing",
    "Colour Out Of Range",
    "Unknown Config Type",

    "Id Missing",
    "Invalid Id",
    "Id Not Found",
    "Invalid Number",
    "Invalid Bool",
    "Invalid String",
    "Invalid Colour"};

// Forward declaration
struct BaseConfig;

// Callback signature
typedef ConfigManagerUpdateResult (*OnSetCallback)(BaseConfig *, void *);

// ------------------------------------------------------------
// Base/common config variables
// ------------------------------------------------------------

struct ConfigMetadata
{
    // Group.Label MUST BE UNIQUE (also used as a key for saving to preferences)
    std::string Group;
    std::string Label;
    std::string Description;
    std::string Info;
    ConfigRenderAs RenderAs;
    std::string Unit;
    float Min;
    float Max;
    float uiMin;
    float uiMax;
    float uiStep;

    char *PrefsId; // Some things we want saved in preferences, others should be reset every time device is reset. To save, give a unique prefs name. Max 15 chars.
    OnSetCallback FunctionOnSet;
    std::string Image;         // e.g. mypic.png
    int ImageVariants;         // e.g. 3 -> mypic.0.png mypic.1.png mypic.2.png
    bool ImageSplitVertically; // e.g. min = 0, max = 3 = 4. Split image vertically into 4, should only display 1/4 the image at a time depending if value is 0,1,2 or 3
};

// ------------------------------------------------------------
// POD variables
// ------------------------------------------------------------

// No value etc. for base, just a structure we can cast to to work out type etc.
struct BaseConfig
{
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
};

struct IntConfig
{
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    int Value;
    int DefaultValue;
};

struct BoolConfig
{
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    bool Value;
    bool DefaultValue;
};

// .Value shouldn't be specified in code, it will be allocated at run time (copied from DefaultValue).
// This is to allow us to free it easily if it is replaced e.g. something else is loaded instead from preferences
struct StringConfig
{
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    char *Value = nullptr;
    const char *DefaultValue = nullptr;

    // Don't forget to clean up
    ~StringConfig() { free(Value); }

    void SetValue(const char *src)
    {
        if (Value)
        {
            free(Value);
            Value = nullptr;
        }
        if (!src)
            return;
        size_t len = strlen(src);
        Value = (char *)malloc(len + 1);
        if (Value)
            memcpy(Value, src, len + 1);
    }
};

struct FloatConfig
{
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    float Value;
    float DefaultValue;
};

struct ColorConfig
{
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    uint8_t r, g, b;
    uint8_t defaultR, defaultG, defaultB;
};

struct CheckListItem
{
    bool Value;
    bool DefaultValue;
    std::string Label;
};

struct CheckListConfig
{
    ConfigType Type;
    int Id;
    ConfigMetadata Metadata;
    CheckListItem CheckList[];
};

// ------------------------------------------------------------
// Config Manager itself
// ------------------------------------------------------------

class ConfigManager
{
public:
    // static int ConfigId;
    static BaseConfig **ConfigMap;
    static int ConfigCount;
    static int ConfigCapacity;

    // static void AddConfig(BaseConfig* config);
    static BaseConfig *GetConfig(int id);
    static ConfigManagerUpdateResult AttemptUpdateConfigById(char *id, char *value);
    static ConfigManagerUpdateResult UpdateConfigById(int id, void *value);
    static ConfigManagerUpdateResult UpdateConfig(BaseConfig *base, void *value);

    static void AddConfigArray(BaseConfig **configs, int count);
    static std::ostringstream GetConfigAsJson();
    static void RenderConfigToSerial();
    static void RenderConfigJsonToSerial();

    static void LoadAllConfigFromPrefs();
    static void SaveAllConfigToPrefs();

    // static void SetStringValue(StringConfig *config, char *value);

    template <typename T>
    static void AddConfig(T *config)
    {
        EnsureCapacity();

        // All your config structs begin with Type, Id, Metadata
        BaseConfig *base = reinterpret_cast<BaseConfig *>(config);

        base->Id = ConfigCount;
        ConfigMap[ConfigCount++] = base;

        Serial.printf("🔧 %3d.%s [%s]\n",
                      config->Id,
                      config->Metadata.Label.c_str(),
                      ConfigTypeDescriptions[(int)config->Type]);

        // Check if is also saved in Preferences and load in value if it is
    }

private:
    static void EnsureCapacity();
};
