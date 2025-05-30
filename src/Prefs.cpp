#include <Preferences.h>
#include "Prefs.h"
#include "DeviceConfig.h"

// Define the static member variable
Preferences Prefs::Handler;

void Prefs::Begin() {
    Handler.begin("config", false);  // Open Preferences in read/write mode
}

void Prefs::Save() {
    // Handler.putString("wifi_ssid", wifiSSID);
    // Handler.putString("wifi_pass", wifiPass);
    // Handler.putInt("custom_value", customValue);
    // Handler.println("Settings saved!");
}

void Prefs::Load() {
#ifdef EXTRA_SERIAL_DEBUG
        Serial.println("Loading previous settings...");
#endif

    for (int i = 0; i < AllStats_Count; i++) {
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

void Prefs::SaveEverything() {

}

void Prefs::Close() {
    Handler.end();
}