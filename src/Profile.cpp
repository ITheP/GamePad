#include <Arduino.h>
#include "Profile.h"
#include "Prefs.h"

// Note we save data encrypted

// sdkconfig.defaults <-- create file, add following
// CONFIG_FLASH_ENCRYPTION_ENABLED=y
// CONFIG_FLASH_ENCRYPTION_MODE_DEVELOPMENT=y  ; or RELEASE for production
//
// - Development mode: Allows re-flashing and debugging.
// - Release mode: Locks down access — irreversible and secure.
//
// On first boot:
// - ESP32 checks if the flash encryption key is set.
// - If not, it generates a random AES key and burns it into eFuse.
// - Flash contents are encrypted transparently from then on.
//
// Verify eFuse Key Status
// Python - Use > espefuse.py (included with ESP-IDF tools)
// espefuse.py --port /dev/ttyUSB0 summary
//
// Look for:
// - FLASH_CRYPT_CNT: Non-zero means encryption is active.
// - FLASH_CRYPT_CONFIG: Shows which flash regions are encrypted.
// - KEY0: Should be marked as USED_FOR_FLASH_ENCRYPTION.
//
// - Irreversible: Once the key is burned, it cannot be changed.
// - Secure Boot: Optional but recommended for full protection.
// - Backups: You cannot retrieve the key — if lost, the device is bricked for encrypted data.
//
// Covers saving using Preferences class

// REMINDER: Max Preferences key length = 15 chars
// Each namespace can store ~4000bytes max

Profile::Profile(int id, char icon, String description) : Id(id), Icon(icon), Description(description)
{
    const char *prefix = "Profile.";
    size_t len = strlen(prefix) + snprintf(nullptr, 0, "%d", id) + 1;
    PrefsKey = new char[len]; // Not tracked, we never free this
    snprintf(PrefsKey, len, "%s%d", prefix, id);

    Icon = icon;
    Description = description;
};

const char* CustomDeviceName_Key = "Cust.DeviceName";
const char* WiFi_Name_Key = "WiFi.Name";
const char* WiFi_Password_Key = "WiFi.Password";


void Profile::Save()
{
    Serial.print("Saving Profile " + Description + " to preferences: ");
    Serial.print("DeviceName='" + CustomDeviceName + "', ");
    Serial.print("WiFi_Name='" + WiFi_Name + "', ");
    Serial.println("WiFi_Password='" + WiFi_Password + "'");

    auto &prefs = Prefs::Handler;

    prefs.begin(PrefsKey);

    prefs.putString(CustomDeviceName_Key, CustomDeviceName);
    prefs.putString(WiFi_Name_Key, WiFi_Name);
    prefs.putString(WiFi_Password_Key, WiFi_Password);

    prefs.end();
}

void Profile::Load()
{
    auto &prefs = Prefs::Handler;

    prefs.begin(PrefsKey);

    CustomDeviceName = prefs.getString(CustomDeviceName_Key, "");
    WiFi_Name = prefs.getString(WiFi_Name_Key,"");
    WiFi_Password = prefs.getString(WiFi_Password_Key, "");

    prefs.end();

    Serial.print("Loaded Profile " + Description + " from saved preferences: ");
    Serial.print("DeviceName='" + CustomDeviceName + "', ");
    Serial.print("WiFi_Name='" + WiFi_Name + "', ");
    Serial.println("WiFi_Password='" + WiFi_Password + "'");
}