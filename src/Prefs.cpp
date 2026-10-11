#include <Preferences.h>
#include <sstream>
#include <iostream>
#include "nvs.h"
#include "nvs_flash.h"
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
// - HTTPS Security Certificates
//
// File storage
// - Statistics

uint32_t Prefs::BootCount;

Preferences Prefs::Handler;

void Prefs::Init()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    Serial_INFO;
    Serial.println("⚙️ Setting up Preferences/Config handler...");

    if (Handler.begin("boot", false)) // false = read/write
    {
        BootCount = Handler.getUInt("bootCount", 0);
        BootCount++;
        Handler.putUInt("bootCount", BootCount);
        Handler.end();
    }
    else
    {
        Debug::WarningFlashes(NVSFailed); // Critical hardware fail - completely bail out of starting uup
    }
}

void Prefs::SaveStats()
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

void Prefs::LoadStats()
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
}

static char *ConfigKey = "Config";

// Config is saved to a global configuration - not individual profiles
void Prefs::SaveConfig(BaseConfig *config)
{
    // ---- 0. Validate ---------------------------------------------------
    if (config == nullptr)
    {
        Serial_INFO;
        Serial.println("Unable to save config: config pointer is null");
        return;
    }

    const char *prefsId = config->Metadata.PrefsId;
    if (prefsId == nullptr || prefsId[0] == '\0')
    {
        Serial_INFO;
        Serial.println("Unable to save config " + String(config->Id) +
                       ": PrefsId missing");
        return;
    }

    // ---- 1. Open NVS ---------------------------------------------------
    auto &prefs = Prefs::Handler;
    if (!prefs.begin(ConfigKey, false))
    { // false = read/write
        Serial_INFO;
        Serial.println("Unable to open NVS for config save");
        return;
    }

    // ---- 2. Write by type ----------------------------------------------
    switch (config->Type)
    {
    case ConfigType::Int:
    {
        IntConfig *c = reinterpret_cast<IntConfig *>(config);
        prefs.putInt(prefsId, c->Value);
        break;
    }

    case ConfigType::Bool:
    {
        BoolConfig *c = reinterpret_cast<BoolConfig *>(config);
        prefs.putBool(prefsId, c->Value);
        break;
    }

    case ConfigType::Float:
    {
        FloatConfig *c = reinterpret_cast<FloatConfig *>(config);
        prefs.putFloat(prefsId, c->Value);
        break;
    }

    case ConfigType::String:
    {
        StringConfig *c = reinterpret_cast<StringConfig *>(config);
        // c->Value is char*; guard against null so putString gets a valid c-string.
        prefs.putString(prefsId, c->Value ? c->Value : "");
        break;
    }

    case ConfigType::Colour:
    {
        ColorConfig *c = reinterpret_cast<ColorConfig *>(config);
        // Pack RGB into 0x00RRGGBB
        uint32_t packed = ((uint32_t)c->r << 16) | ((uint32_t)c->g << 8) | ((uint32_t)c->b);
        prefs.putUInt(prefsId, packed);
        break;
    }

    default:
        Serial_INFO;
        Serial.println("Unable to save config " + String(prefsId) +
                       ": unknown type specified");
        break;
    }

    prefs.end();
}

void Prefs::LoadConfig(BaseConfig *config)
{
    // ---- 0. Validate ---------------------------------------------------
    if (config == nullptr)
    {
        return;
    }

    const char *prefsId = config->Metadata.PrefsId;
    if (prefsId == nullptr || prefsId[0] == '\0')
    {
        return; // nothing to key off — skip silently
    }

    // ---- 1. Open NVS (read-only) ---------------------------------------
    auto &prefs = Prefs::Handler;
    if (!prefs.begin(ConfigKey, true))
    {           // true = read-only
        return; // no stored config yet — keep defaults
    }

    // ---- 2. Read by type -----------------------------------------------
    switch (config->Type)
    {
    case ConfigType::Int:
    {
        IntConfig *c = reinterpret_cast<IntConfig *>(config);
        c->Value = prefs.getInt(prefsId, c->Value);
        break;
    }

    case ConfigType::Bool:
    {
        BoolConfig *c = reinterpret_cast<BoolConfig *>(config);
        c->Value = prefs.getBool(prefsId, c->Value);
        break;
    }

    case ConfigType::Float:
    {
        FloatConfig *c = reinterpret_cast<FloatConfig *>(config);
        c->Value = prefs.getFloat(prefsId, c->Value);
        break;
    }

    case ConfigType::String:
    {
        StringConfig *c = reinterpret_cast<StringConfig *>(config);
        String loaded = prefs.getString(prefsId, String());
        if (loaded.length() > 0)
        {
            c->SetValue(loaded.c_str());
            //ConfigManager::SetStringValue(c, (char *)(loaded.c_str()));
        }
        break;
    }

    case ConfigType::Colour:
    {
        ColorConfig *c = reinterpret_cast<ColorConfig *>(config);
        // 0xFFFFFFFF is a "not present" sentinel — no valid colour packs to it.
        // WARNING: If ever using alpha channel and wanting the 0xFFFFFFFF number we may need to do this differently
        uint32_t packed = prefs.getUInt(prefsId, 0xFFFFFFFF);
        if (packed != 0xFFFFFFFF)
        {
            c->r = (uint8_t)((packed >> 16) & 0xFF);
            c->g = (uint8_t)((packed >> 8) & 0xFF);
            c->b = (uint8_t)(packed & 0xFF);
        }
        break;
    }

    default:
        break;
    }

    prefs.end();
}

// Dump all keys and values from the ConfigKey namespace
void Prefs::RenderPrefsConfigToSerial()
{
    const char *namespaceName = "Config";
    const char *partitionName = "nvs";

    nvs_iterator_t it = nvs_entry_find(partitionName, namespaceName, NVS_TYPE_ANY);

    if (it == nullptr) {
        Serial.println("No keys found in namespace");
        return;
    }

    auto &prefs = Prefs::Handler;
    if (!prefs.begin(namespaceName, true)) {   // read-only
        Serial.println("Failed to open Preferences namespace");
        nvs_release_iterator(it);
        return;
    }

    Serial_INFO;
    Serial.println("🧠⚙️ Config (NVS saved Prefs)");

    while (it != nullptr) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        switch (info.type) {
            case NVS_TYPE_I32:
                Serial.printf("🧠#️⃣ %-15s: %d\n", info.key, prefs.getInt(info.key));
                break;
            case NVS_TYPE_U32:
                Serial.printf("🧠#️⃣ %-15s: %u\n", info.key, prefs.getUInt(info.key));
                break;
            case NVS_TYPE_I8:
                Serial.printf("🧠🔤 %-15s: %d\n", info.key, prefs.getChar(info.key));
                break;
            case NVS_TYPE_U8:
                Serial.printf("🧠🔤 %-15s: %u\n", info.key, prefs.getUChar(info.key));
                break;
            case NVS_TYPE_I16:
                Serial.printf("🧠#️⃣ %-15s: %d\n", info.key, prefs.getShort(info.key));
                break;
            case NVS_TYPE_U16:
                Serial.printf("🧠#️⃣ %-15s: %u\n", info.key, prefs.getUShort(info.key));
                break;
            case NVS_TYPE_STR:
                Serial.printf("🧠📝 %-15s: %s\n", info.key,
                              prefs.getString(info.key, String()).c_str());
                break;
            case NVS_TYPE_BLOB:
                Serial.printf("🧠❔ %-15s: <blob, %u bytes>\n", info.key,
                              prefs.getBytesLength(info.key));
                break;
            default:
                Serial.printf("🧠❔ %-15s: <type %d>\n", info.key, info.type);
                break;
        }

        it = nvs_entry_next(it);
    }

    nvs_release_iterator(it);
    prefs.end();
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

// HTTPS Certificate management functions
bool Prefs::LoadHTTPSCertificates(std::string &certPem, std::string &keyPem)
{
    bool success = false;

    if (Handler.begin("https_certs", true)) // true = read-only
    {
        size_t certSize = Handler.getBytesLength("cert");
        size_t keySize = Handler.getBytesLength("key");

        if (certSize > 0 && keySize > 0)
        {
            char *certBuffer = new char[certSize + 1];
            char *keyBuffer = new char[keySize + 1];

            size_t readCertSize = Handler.getBytes("cert", (uint8_t *)certBuffer, certSize);
            size_t readKeySize = Handler.getBytes("key", (uint8_t *)keyBuffer, keySize);

            if (readCertSize == certSize && readKeySize == keySize)
            {
                certBuffer[certSize] = '\0';
                keyBuffer[keySize] = '\0';
                certPem = std::string(certBuffer);
                keyPem = std::string(keyBuffer);
                success = true;
            }

            delete[] certBuffer;
            delete[] keyBuffer;
        }

        Handler.end();
    }

    return success;
}

void Prefs::SaveHTTPSCertificates(const std::string &certPem, const std::string &keyPem)
{
    if (Handler.begin("https_certs", false)) // false = read/write
    {
        Handler.putBytes("cert", (const uint8_t *)certPem.c_str(), certPem.length());
        Handler.putBytes("key", (const uint8_t *)keyPem.c_str(), keyPem.length());
        Handler.end();

#ifdef EXTRA_SERIAL_DEBUG
        Serial.println("✅ HTTPS certificates saved to NVS");
#endif
    }
}