#include "SerialDebug.h"
#include "Battery.h"
#include "Prefs.h"

// Forward declarations of handlers
static void HandleHelp(const std::string &param);

// Command list here
SerialDebugFunction SerialDebug::SerialDebugFunctions[] = {
    {Description : "General"}, // Heading

    {"Help",
     "Show This Help.\n  Help <command>            - ...for more detailed information about a specific command",
     "",
     &SerialDebug::ShowHelp},

    {"Hello",
     "Shows a little hello 😎",
     "",
     &SerialDebug::ShowHello},

    {"Clear",
     "Clears Serial window (if client allows)",
     "",
     &SerialDebug::ClearSerial},


    {Description : "Power"}, // Heading

    {"Power State",
     "Show Power State",
     "Includes battery and USB power information, hardware pin readings, etc.",
     &Battery::PrintPowerStateToSerial},

    {Description : "Config"}, // Heading

    {"Show Config",
     "Show runtime configurable settings",
     "Specifically live config settings, definable at run time, and exposed via the web interface. Doesn't include all settings, hardcoded defaults etc.",
     &ConfigManager::RenderConfigToSerial},

    {"Show Config JSON",
     "Show runtime configurable settings in JSON format",
     "Specifically live config settings, definable at run time, and exposed via the web interface. Doesn't include all settings, hardcoded defaults etc. Output shown is in JSON format, as supplied via web api to frontend web.",
     &ConfigManager::RenderConfigJsonToSerial},

    {"Show Prefs Config",
     "Show Prefs saved Config",
     "Config entries that have been saved into Prefs/Preferences. Will be loaded when device boots. Are saved when web site updates relevant Config entries.",
     &Prefs::RenderPrefsConfigToSerial},

    {"Save Config To Prefs",
     "Saves all relevant Config items to Preferences",
     "Only Config items with a PrefsId defined will be saved",
     &ConfigManager::SaveAllConfigToPrefs},

    {"Load Config From Prefs",
     "Loads all relevant Config items from Preferences",
     "Only Config items with a PrefsId defined will be saved",
     &ConfigManager::LoadAllConfigFromPrefs},

    {Description : "Inputs"}, // Heading

    {"Live Input",
     "Toggle showing live state of inputs",
     "Will display all digital, analog, virtual, hat, pulse, etc. inputs, their states, readings, values etc. Sends a serial command to reset display to top left (in attempt to have a nice readable static display area). Don't expect to see much else over the serial output while this is on.",
     &ConfigInputSerialLiveOutput_ToggleState},

    {"Live Input Slow",
     "Set throttling to slow speed",
     "Makes sure Live Input display is enabled, along with slow speed update rate. Makes serial output more readable e.g. when tuning hardware.",
     &ConfigInputSerialLiveOutputThrottle_Slow},

    {"Live Input Medium",
     "Set throttling to medium speed",
     "Makes sure Live Input display is enabled, along with medium speed update rate. Makes serial output more readable e.g. when tuning hardware.",
     &ConfigInputSerialLiveOutputThrottle_Medium},

    {"Live Input Fast",
     "Set throttling to fast speed",
     "Makes sure Live Input display is enabled, along with fast update rate. Makes serial output more readable e.g. when tuning hardware.",
     &ConfigInputSerialLiveOutputThrottle_Fast},

    {"Live Input Max",
     "No throttling, fast as possible",
     "Makes sure Live Input display is enabled, with no throttling of update speed. Hope your serial client can cope!",
     &ConfigInputSerialLiveOutputThrottle_Fast},

    {Description : "Display / Screen"}, // Heading

    {"FPS",
     "Toggle on/off the display of FPS",
     "",
     &ConfigForceFPSDisplay_ToggleState},

    {"White Screen",
     "Toggle on/off the display to show a completely white screen",
     "Switches the display to a completely white screen, useful for physical alignment when fitting the screen panel into a device - you can see where the visible edges are.",
     &ConfigScreenWhite_ToggleState},

    {Description : "LEDs"}, // Heading

    {"Set Brightness",
     "Global LED brightness",
     "Sets LED brightness to the given integer, 0-255.\n  Example: Set Brightness 200",
     nullptr,
     SerialDebugFunctionParameterType::Int,
     &SerialDebugTrampoline<int, ApplyGlobalLEDBrightnessToConfig>},

    {Description : "Crash Handling"}, // Heading

    {"Show Crash Logs",
     "Show saved crash logs, if available.",
     "List of saved crash logs, plus contents of latest 2. Full crash log details are also available via the web interface.",
     &Debug::RenderCrashLogsToSerial},

    {"Crash Device",
     "Attempt to crash the device",
     "Specifically attempts to crash the device, not just reset it. Good for testing crash handling. Note that some documented crash mechanisms don't always work! Current methods include accessing prohibited and invalid memory, and divide by zero.",
     &Debug::CrashDeviceOnPurpose},
};

// Count entries
const int SerialDebug::SerialDebugFunctionsCount =
    sizeof(SerialDebug::SerialDebugFunctions) / sizeof(SerialDebugFunction);

// Lookup dictionary
std::unordered_map<std::string, SerialDebugFunction *> SerialDebug::FunctionMap;
std::vector<SerialDebugFunction *> SerialDebug::FunctionOrder;

// --------------------------------
// Initialise dictionary from array
// --------------------------------
void SerialDebug::Init()
{
    AddSerialDebugFunctions(SerialDebugFunctions, SerialDebugFunctionsCount);
    AddSerialDebugFunctions(ConfigManager_SerialDebugFunctions, ConfigManager_SerialDebugFunctions_Count);

    Serial_INFO;
    Serial.println("🔌 SerialDebug Initialised. Enter 'Help' for commands.");
}

void SerialDebug::AddSerialDebugFunctions(SerialDebugFunction serialDebugFunctions[], int count)
{
    for (int i = 0; i < count; i++)
    {
        auto &entry = serialDebugFunctions[i];

        // Every entry goes into the ordered list (for when help is called), so headers appear too.
        FunctionOrder.push_back(&entry);

        // Ignore headers in actual map, we don't care
        if (entry.Instruction.empty())
            continue;

        // Case insensitive lookup - convert to lowercase
        std::string key = entry.Instruction;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        FunctionMap[key] = &entry;
    }
}

// --------------------------------
// ShowHelp handler
// --------------------------------
void SerialDebug::ShowHelp()
{
    Serial.println("Available Commands:");

    for (auto *entry : FunctionOrder)
    {
        if (entry->Instruction.empty())
        {
            Serial.printf("\n%s...\n", entry->Description.c_str());
        }
        else
        {
            Serial.printf("  %-25s - %s\n",
                          entry->Instruction.c_str(),
                          entry->Description.c_str());
        }
    }
}

void SerialDebug::ShowHello()
{
    static int callCount = 0;
    ++callCount;

    static const char *greetings[] = {
        // Initial greetings
        "👋 Hello to you too my friend!",
        "👋 Well hello there!",
        "❤️ Hi! Good to see you.",
        "🖥️ Greetings, human.",
        "⚓ Ahoy!",
        "😊 Hey there, welcome!",
        "❤️ Hello, nice to meet you.",
        "👀 Hiya! What can I do for you?",
        "😁 Good day to you!",
        "🙂 Salutations!",

        // Repeat-visitor greetings
        "👋 Hello again! Back so soon?",
        "👋 You're back! Hello once more.",
        "🎉 Well, if it isn't you again. Hello!",
        "😁 Hello yet again, my persistent friend.",
        "👀 Still here? Hello again!",
        "👀 You really like saying hello, don't you?",
        "❤️ Hello again — I was just thinking about you.",
        "🎁 Another hello? You spoil me!",
        "🪖 Shall we play a game? ... Only joking!",
        "🎵 Hello again, you're back once more,\n Lot's of times knocking at my door,\n Keep it coming cause I don't mind,\n I'll sing this greeting 'til you get bored and stop 😎"};

    constexpr int total = sizeof(greetings) / sizeof(greetings[0]);
    constexpr int firstHalf = total / 2;

    int index = (callCount <= 2)
                    ? random(firstHalf) // pick from first half only
                    : random(total);    // pick from whole array

    Serial.println(greetings[index]);
}

/// @brief Clears Serial window in client (if client supports relevant codes)
void SerialDebug::ClearSerial()
{
    Serial.print("\033[3J\033[2J\033[H");
}

// ------------------------------------------------------------------------
// Parameter parser
//
// Attempts to strip one parameter from the end of `raw`. On success,
// fills `remainingKey`, `type`, and one of the typed outputs, and returns
// true. On failure, returns false and leaves outputs untouched.
// ------------------------------------------------------------------------
static bool TryParseParameter(const std::string &raw,
                              std::string &remainingKey,
                              SerialDebugFunctionParameterType &type,
                              int &intOut,
                              float &floatOut,
                              bool &boolOut,
                              std::string &stringOut)
{
    std::string s = raw;

    // Trim trailing whitespace
    while (!s.empty() && isspace((unsigned char)s.back()))
        s.pop_back();
    if (s.empty())
        return false;

    // ---- Quoted string at end ----
    if (s.back() == '"')
    {
        size_t closePos = s.size() - 1;
        size_t openPos = s.rfind('"', closePos - 1);
        if (openPos != std::string::npos)
        {
            bool atStart = (openPos == 0);
            bool precededBySpace = !atStart && isspace((unsigned char)s[openPos - 1]);

            if (atStart || precededBySpace)
            {
                stringOut = s.substr(openPos + 1, closePos - openPos - 1);
                remainingKey = s.substr(0, openPos);
                while (!remainingKey.empty() && isspace((unsigned char)remainingKey.back()))
                    remainingKey.pop_back();
                type = SerialDebugFunctionParameterType::String;
                return true;
            }
        }
    }

    // ---- Last whitespace-separated token ----
    size_t lastSpace = s.find_last_of(" \t");
    std::string token = (lastSpace == std::string::npos) ? s : s.substr(lastSpace + 1);
    if (token.empty())
        return false;

    // Bool
    std::string lowered = token;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);
    if (lowered == "true" || lowered == "false")
    {
        boolOut = (lowered == "true");
        remainingKey = (lastSpace == std::string::npos) ? "" : s.substr(0, lastSpace);
        type = SerialDebugFunctionParameterType::Bool;
        return true;
    }

    // Int
    errno = 0;
    char *end = nullptr;
    long iv = strtol(token.c_str(), &end, 10);
    if (end != token.c_str() && *end == '\0' && errno != ERANGE)
    {
        intOut = (int)iv;
        remainingKey = (lastSpace == std::string::npos) ? "" : s.substr(0, lastSpace);
        type = SerialDebugFunctionParameterType::Int;
        return true;
    }

    // Float
    errno = 0;
    end = nullptr;
    float fv = strtof(token.c_str(), &end);
    if (end != token.c_str() && *end == '\0' && errno != ERANGE)
    {
        floatOut = fv;
        remainingKey = (lastSpace == std::string::npos) ? "" : s.substr(0, lastSpace);
        type = SerialDebugFunctionParameterType::Float;
        return true;
    }

    return false;
}

// ------------------------------------------------------------------------
// CheckSerialInput — dispatch
// ------------------------------------------------------------------------
void SerialDebug::CheckSerialInput()
{
    static String buffer = "";

    while (Serial.available())
    {
        char c = Serial.read();

        if (c != '\n' && c != '\r')
        {
            buffer += c;
            continue;
        }

        if (buffer.length() == 0)
            continue;

        // Different modes we can use for serial handling to aid visibility
        //Serial.println();
        ClearSerial();

        std::string line = buffer.c_str();
        buffer = "";

        // 1. Try to peel a parameter off the end.
        std::string keyPart = line;
        SerialDebugFunctionParameterType paramType = SerialDebugFunctionParameterType::None;
        int intVal = 0;
        float floatVal = 0.0f;
        bool boolVal = false;
        std::string stringVal;

        TryParseParameter(line, keyPart, paramType,
                          intVal, floatVal, boolVal, stringVal);

        // 2. Normalise the key.
        std::string key = keyPart;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        // 3. "help" / "help <command>" special case.
        // Must run before the "unknown command" lookup, and must catch both
        // the bare "help" and "help <anything>" forms.
        {
            // Peek at the first word only — case-insensitively.
            std::string firstWord = line;
            size_t sp = line.find(' ');
            if (sp != std::string::npos)
                firstWord = line.substr(0, sp);

            std::string lowerFirst = firstWord;
            std::transform(lowerFirst.begin(), lowerFirst.end(),
                           lowerFirst.begin(), ::tolower);

            if (lowerFirst == "help" || lowerFirst == "?")
            {
                // Bare "help" → full list
                if (sp == std::string::npos)
                {
                    ShowHelp();
                    return;
                }

                // "help <something>" → detailed help for that command
                std::string target = line.substr(sp + 1);
                while (!target.empty() && isspace((unsigned char)target.back()))
                    target.pop_back();

                std::string lowerTarget = target;
                std::transform(lowerTarget.begin(), lowerTarget.end(),
                               lowerTarget.begin(), ::tolower);

                auto it2 = FunctionMap.find(lowerTarget);
                if (it2 == FunctionMap.end())
                {
                    Serial.printf("No help available for '%s', command not found\n",
                                  target.c_str());
                }
                else
                {
                    auto *h = it2->second;
                    Serial.printf("⚙️ %s\nℹ️ %s\n", h->Instruction.c_str(), h->Description.c_str());
                    if (!h->Details.empty())
                        Serial.printf("ℹ️ %s\n", h->Details.c_str());
                }
                return;
            }
        }

        // 4. Look up the command.
        auto it = FunctionMap.find(key);
        if (it == FunctionMap.end())
        {
            Serial.printf("Unknown command: '%s'\n", key.c_str());
            return;
        }

        SerialDebugFunction *entry = it->second;

        // 5. Dispatch.

        // 5a. No-parameter command.
        if (!entry->HasParameter())
        {
            if (paramType != SerialDebugFunctionParameterType::None)
            {
                Serial_INFO;
                Serial.printf("Note: '%s' does not take a parameter; ignoring '%s'\n",
                              entry->Instruction.c_str(),
                              line.substr(keyPart.size() + 1).c_str());
            }

            if (entry->Function)
                entry->Function();
            else
            {
                Serial_ERROR;
                Serial.printf("Command '%s' has no function handler.\n",
                              entry->Instruction.c_str());
            }
            return;
        }

        // 5b. Parameterised command but none supplied → show help.
        if (paramType == SerialDebugFunctionParameterType::None)
        {
            Serial.printf("%s - %s\n",
                          entry->Instruction.c_str(),
                          entry->Description.c_str());
            if (!entry->Details.empty())
                Serial.printf("%s\n", entry->Details.c_str());
            return;
        }

        // 5c. Type mismatch.
        if (paramType != entry->ParameterType)
        {
            const char *expected = "unknown";
            switch (entry->ParameterType)
            {
            case SerialDebugFunctionParameterType::Int:
                expected = "int";
                break;
            case SerialDebugFunctionParameterType::Float:
                expected = "float";
                break;
            case SerialDebugFunctionParameterType::Bool:
                expected = "bool";
                break;
            case SerialDebugFunctionParameterType::String:
                expected = "string";
                break;
            default:
                break;
            }
            Serial_ERROR;
            Serial.printf("Parameter type mismatch for '%s': expected %s\n",
                          entry->Instruction.c_str(), expected);
            return;
        }

        // 5d. Invoke the trampoline with a pointer to the correctly-typed value.
        switch (entry->ParameterType)
        {
        case SerialDebugFunctionParameterType::Int:
        {
            int v = intVal;
            entry->ParameterFunction(&v);
            break;
        }
        case SerialDebugFunctionParameterType::Float:
        {
            float v = floatVal;
            entry->ParameterFunction(&v);
            break;
        }
        case SerialDebugFunctionParameterType::Bool:
        {
            bool v = boolVal;
            entry->ParameterFunction(&v);
            break;
        }
        case SerialDebugFunctionParameterType::String:
        {
            std::string v = stringVal;
            entry->ParameterFunction(&v);
            break;
        }
        default:
            break;
        }
    }
}