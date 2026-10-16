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
    Color,
    CheckList
};

static const char *ConfigTypeDescriptions[] = {
    "Int",
    "Bool",
    "Float",
    "String",
    "Colour",
    "CheckList"};

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
    /// <summary>
    /// Group label. Used to group config items in web and in serial output.
    /// e.g. "LED" for LED related settings
    /// </summary>
    std::string Group;

    /// <summary>
    /// Display label
    /// e.g. "Brightness" for LED Brightness setting in LED group.
    /// </summary>
    std::string Label;

    /// <summary>
    /// Short description shown to the user.
    /// e.g. "Global brightness of LED's"
    /// </summary>
    std::string Description;

    /// <summary>
    /// Additional info text.
    /// e.g. "Very low brightness levels may result in funny looking LED colours or fades as there isn't the resolution of brightness levels to represent subtle differences in colour"
    /// </summary>
    std::string Info;

    /// <summary>
    /// Hint on how this config item should be rendered in a UI.
    /// Not needed most of the time, defaults to "Default"
    /// </summary>
    ConfigRenderAs RenderAs;

    /// <summary>
    /// Unit string displayed next to the value
    /// e.g. "Hz", "ms", "Seconds"
    /// </summary>
    std::string Unit;

    /// <summary>
    /// Minimum allowed value. May be..
    /// - Int value e.g. 20
    /// - Float value e.g. 0.5
    /// - String minimum character length e.g. 10
    /// </summary>
    float Min;

    /// <summary>
    /// Maximum allowed value.
    /// - Int value e.g. 200
    /// - Float value e.g. 1.5
    /// - String maximum character length e.g. 100
    /// </summary>
    float Max;

    /// <summary>
    /// Minimum value shown in the UI.
    /// Similar to minimum value, except optimised for UI (e.g. sensible ranges rather than theoretical max)
    /// </summary>
    float uiMin;

    /// <summary>
    /// Maximum value shown in the UI.
    /// Similar to maximum value, except optimised for UI (e.g. sensible ranges rather than theoretical max)
    /// </summary>
    float uiMax;

    /// <summary>
    /// Step size hint for e.g. sliders in UI components
    /// e.g. 0.1, 10, 250, etc.
    /// </summary>
    float uiStep;

    /// <summary>
    /// Some config settings are (or should be) reset every time device is reset.
    /// Others we want to save permanently so they survive a device reset.
    /// To save, give a unique prefs name. Max 15 chars. Name must be unique across all Config settings.
    /// e.g. "HW.Debounce"
    /// </summary>
    char *PrefsId = nullptr;

    /// <summary>
    /// Callback invoked when this value is set.
    /// Note that when used, the function called must also set the Config.Value, it will not be done automatically.
    /// If no function is set, then .Value will automatically update when changed via ConfigManager::UpdateConfig(...)
    /// </summary>
    OnSetCallback FunctionOnSet;

    /// <summary>
    /// Image filename, e.g. "mypic.png" - may be used by web ui. Assume image must exist within /data/img/ for the web service to access
    /// </summary>
    std::string Image;

    /// <summary>
    /// Number of image variants. If <= 1 then any image specified will be used as is. For values > 1 then a random variant will be defined.
    /// Allows for some variance in web UI.
    /// e.g. 3 -> mypic.0.png mypic.1.png mypic.2.png
    /// </summary>
    int ImageVariants;

    /// <summary>
    /// Split image vertically into 4 separate images. i.e. Image Sprites.
    /// Based on number of values from min to max.
    /// e.g. Min = 0, Max = 3, ImageSplitVertically = true, the web UI can map a value 0->3 to corresponding slices of the image.
    /// </summary>
    bool ImageSplitVertically;
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
    ConfigType Type = ConfigType::Int;
    int Id;
    ConfigMetadata Metadata;
    int Value;
    int DefaultValue;
};

struct BoolConfig
{
    ConfigType Type = ConfigType::Bool;
    int Id;
    ConfigMetadata Metadata;
    bool Value;
    bool DefaultValue;
};

// For StringConfig, .Value shouldn't be specified in code, it will be allocated at run time (copied from DefaultValue).
// This is to allow us to free it easily if it is replaced e.g. something else is loaded instead from preferences
struct StringConfig
{
    ConfigType Type = ConfigType::String;
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
    ConfigType Type = ConfigType::Float;
    int Id;
    ConfigMetadata Metadata;
    float Value;
    float DefaultValue;
};

struct ColorConfig
{
    ConfigType Type = ConfigType::Color;
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
    ConfigType Type = ConfigType::CheckList;
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
    static BaseConfig **ConfigMap;
    static int ConfigCount;
    static int ConfigCapacity;

    static BaseConfig *GetConfig(int id);
    static ConfigManagerUpdateResult AttemptUpdateConfigById(char *id, char *value);
    static ConfigManagerUpdateResult UpdateConfigById(int id, void *value);
    static ConfigManagerUpdateResult UpdateConfig(BaseConfig *base, void *value);

    static void AddConfigArray(BaseConfig **configs, int count);
    static std::ostringstream GetConfigAsJson();
    static void RenderConfigToSerial();
    static void RenderConfigJsonToSerial();

    static std::string GetConfigValueAsString(BaseConfig *config);

    static void LoadAllConfigFromPrefs();
    static void SaveAllConfigToPrefs();

    template <typename T>
    static void AddConfig(T *config)
    {
        EnsureCapacity();

        // All your config structs begin with Type, Id, Metadata
        BaseConfig *base = reinterpret_cast<BaseConfig *>(config);

        base->Id = ConfigCount;
        ConfigMap[ConfigCount++] = base;

        Serial.printf("🔧 [%-3d] %-30s -> %s (%s)\n",
                      config->Id,
                      config->Metadata.Label.c_str(),
                      GetConfigValueAsString(config).c_str(),
                      ConfigTypeDescriptions[(int)config->Type]);
    }

private:
    static void EnsureCapacity();
};
