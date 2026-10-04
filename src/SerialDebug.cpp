#include "SerialDebug.h"
#include "Battery.h"

// Forward declarations of handlers
static void HandleHelp(const std::string& param);

// -----------------------------
// Define your command list here
// -----------------------------
SerialDebugFunction SerialDebug::SerialDebugFunctions[] = {
    {
        "Help",
        "Show This Help",
        "Show all available instructions. Use: Help <command> for detailed info.",
        &SerialDebug::ShowHelp
    },
    {
        "PowerState",
        "Show Power State",
        "Show the current power state, including battery and USB power information, hardware pin readings, etc.",
        &Battery::PrintPowerStateToSerial
    },
     
};

// Count entries
const int SerialDebug::SerialDebugFunctionsCount =
    sizeof(SerialDebug::SerialDebugFunctions) / sizeof(SerialDebugFunction);

// Lookup dictionary
std::unordered_map<std::string, SerialDebugFunction*> SerialDebug::FunctionMap;

// --------------------------------
// Initialise dictionary from array
// --------------------------------
void SerialDebug::Init()
{
    for (int i = 0; i < SerialDebugFunctionsCount; i++)
    {
        auto& entry = SerialDebugFunctions[i];

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
        for (int i = 0; i < SerialDebugFunctionsCount; i++)
        {
            auto& entry = SerialDebugFunctions[i];
            Serial.printf("  %-10s - %s\n",
                          entry.Instruction.c_str(),
                          entry.Description.c_str());
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

            std::string key = cmd;
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            // Lookup
            auto it = FunctionMap.find(key);
            if (it == FunctionMap.end())
            {
                Serial.printf("Unknown command: '%s'\n", cmd.c_str());
                return;
            }

            // Call handler
            SerialDebugFunction* entry = it->second;
            
            // Parameter and is help? Special case for displaying detailed help for a specific command
            if ((key == "help" || key == "?") && !param.empty()) {
                // Show detailed help for a specific command
                std::string paramKey = param;
                std::transform(paramKey.begin(), paramKey.end(), paramKey.begin(), ::tolower);
                auto it2 = FunctionMap.find(paramKey);

                if (it2 == FunctionMap.end())
                {
                    Serial.printf("No help available for '%s', command not found\n", param.c_str());
                    return;
                }

                auto* helpEntry = it2->second;
                Serial.printf("Instruction: %s\n", helpEntry->Instruction.c_str());
                Serial.printf("Description: %s\n", helpEntry->Description.c_str());
                Serial.printf("Details: %s\n", helpEntry->Details.c_str());
            }
            else {
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
