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
bool Prefs::LoadHTTPSCertificates(std::string &certPem, std::string &keyPem) {
    bool success = false;

    if (Handler.begin("https_certs", true))  // true = read-only
    {
        size_t certSize = Handler.getBytesLength("cert");
        size_t keySize = Handler.getBytesLength("key");

        if (certSize > 0 && keySize > 0) {
            char *certBuffer = new char[certSize + 1];
            char *keyBuffer = new char[keySize + 1];

            size_t readCertSize = Handler.getBytes("cert", (uint8_t *)certBuffer, certSize);
            size_t readKeySize = Handler.getBytes("key", (uint8_t *)keyBuffer, keySize);

            if (readCertSize == certSize && readKeySize == keySize) {
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

void Prefs::SaveHTTPSCertificates(const std::string &certPem, const std::string &keyPem) {
    if (Handler.begin("https_certs", false))  // false = read/write
    {
        Handler.putBytes("cert", (const uint8_t *)certPem.c_str(), certPem.length());
        Handler.putBytes("key", (const uint8_t *)keyPem.c_str(), keyPem.length());
        Handler.end();

#ifdef EXTRA_SERIAL_DEBUG
        Serial.println("✅ HTTPS certificates saved to NVS");
#endif
    }
}