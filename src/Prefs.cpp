#include <Preferences.h>
#include <sstream>
#include <iostream>
#include "Prefs.h"
#include "DeviceConfig.h"
#include "Debug.h"

// Prefs are device specific, rather than profile which are multiple. E.g. individual profiles have different WiFi settings,
// but the Prefs include general device settings, overall statistics, etc.

// NOTE: Some Preferences/settings are stored in NVS storage, some (bigger stuff) is stored in LittleFS files
//
// NVS
// - Wifi
// - Bluetooth
// - General configuration settings
//
// File storage
// - Statistics

uint32_t Prefs::BootCount;

Preferences Prefs::Handler;

void Prefs::Init()
{
    if (Handler.begin("boot", false))       // false = read/write
    {
        BootCount = Handler.getUInt("bootCount", 0);
        BootCount++;
        Handler.putUInt("bootCount", BootCount);
        Handler.end();
    }
    else
    {
        Debug::WarningFlashes(NVSFailed);   // Critical hardware fail - completely bail out of starting uup
    }
}

void Prefs::Save()
{
    #ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Saving settings...");
#endif

    for (int i = 0; i < AllStats_Count; i++)
    {
#ifdef EXTRA_SERIAL_DEBUG
        Serial.println("...Statistics: " + String(AllStats[i]->Description));
#endif

        AllStats[i]->SaveToPreferences();
    }
}

void Prefs::Load()
{
#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Loading previous settings...");
#endif

    for (int i = 0; i < AllStats_Count; i++)
    {
#ifdef EXTRA_SERIAL_DEBUG
        Serial.println("...Statistics: " + String(AllStats[i]->Description));
#endif

        AllStats[i]->LoadFromPreferences();
    }

    // String wifiSSID = Handler.getString("wifi_ssid", "defaultSSID");
    // String wifiPass = Handler.getString("wifi_pass", "defaultPass");
    // int customValue = Handler.getInt("custom_value", 42);  // Default if not set

    // Serial.println("Loaded Settings:");
    // Serial.println("SSID: " + wifiSSID);
    // Serial.println("Password: " + wifiPass);
    // Serial.println("Custom Value: " + String(customValue));
}

void Prefs::WebDebug(std::ostringstream *stream)
{
    *stream << "<table border='1'>"
           << "<thead><tr>"
           << "<th rowspan='2'>Name</th>"
           << "<th colspan='4'>Value</th>"
           << "</tr></thead><tbody>";

    for (int i = 0; i < AllStats_Count; i++)
        AllStats[i]->WebDebug(stream);

    *stream << "</tbody></table>";
}

void Prefs::SaveEverything()
{
}

void Prefs::Close()
{
    Handler.end();
}