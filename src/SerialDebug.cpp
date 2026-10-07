#include "SerialDebug.h"
#include "Battery.h"

// Forward declarations of handlers
static void HandleHelp(const std::string &param);

// Command list here
SerialDebugFunction SerialDebug::SerialDebugFunctions[] = {
    {Description : "General"}, // Heading

    {"Help",
     "Show This Help.\n  Help <command>       - ...for more detailed information about a specific command",
     "",
     &SerialDebug::ShowHelp},

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
static std::vector<SerialDebugFunction *> FunctionOrder;

// --------------------------------
// Initialise dictionary from array
// --------------------------------
void SerialDebug::Init()
{
    AddSerialDebugFunctions(SerialDebugFunctions, SerialDebugFunctionsCount);
    AddSerialDebugFunctions(ConfigManager_SerialDebugFunctions, ConfigManager_SerialDebugFunctions_Count);
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

    Serial.println("SerialDebug: Ready. Type 'Help' for commands.");
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
            Serial.printf("  %-20s - %s\n",
                          entry->Instruction.c_str(),
                          entry->Description.c_str());
        }
    }
}

// --------------------------------
// CheckSerialInput()
// --------------------------------
void SerialDebug::CheckSerialInput()
{
    static String buffer = "";

    while (Serial.available())
    {
        char c = Serial.read();

        if (c == '\n' || c == '\r')
        {
            if (buffer.length() == 0)
                return;

            // Convert to std::string
            std::string line = buffer.c_str();
            buffer = "";

            // Split into command + optional parameter
            std::string cmd;
            std::string param;

            size_t spacePos = line.find(' ');
            if (spacePos == std::string::npos)
            {
                cmd = line;
            }
            else
            {
                cmd = line.substr(0, spacePos);
                param = line.substr(spacePos + 1);
            }

            // If cmd == "help" or "?" then param is the command to get help for
            // otherwise we use the whole line as the command

            std::string key = cmd;
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            // Parameter and is help? Special case for displaying detailed help for a specific command
            if ((key == "help" || key == "?") && !param.empty())
            {
                // Show detailed help for a specific command
                std::string paramKey = param;
                std::transform(paramKey.begin(), paramKey.end(), paramKey.begin(), ::tolower);
                auto it2 = FunctionMap.find(paramKey);

                if (it2 == FunctionMap.end())
                {
                    Serial.printf("No help available for '%s', command not found\n", param.c_str());
                }
                else
                {
                    auto *helpEntry = it2->second;
                    Serial.printf("%s - %s\n", helpEntry->Instruction.c_str(), helpEntry->Description.c_str());
                    if (!helpEntry->Details.empty())
                    {
                        Serial.printf("%s\n", helpEntry->Details.c_str());
                    }
                }

                return;
            }

            // Reuse line as the key, so we can have spaces in commands. Still want lower case
            key = line;
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            // Lookup
            auto it = FunctionMap.find(key);
            if (it == FunctionMap.end())
            {
                Serial.printf("Unknown command: '%s'\n", cmd.c_str());
                return;
            }
            else
            {
                // Call handler
                SerialDebugFunction *entry = it->second;

                // Call a function with a parameter isn't implemented yet, so just call the function
                entry->Function();
            }

            return;
        }
        else
        {
            buffer += c;
        }
    }
}
