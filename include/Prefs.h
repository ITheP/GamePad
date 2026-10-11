#pragma once

#include <Preferences.h>
#include <Arduino.h>
#include <string>
#include "ConfigManager.h"

class Prefs {
public:
    static void Init();
    static void SaveStats();
    static void LoadStats();
    static void LoadConfig(BaseConfig *config);
    static void SaveConfig(BaseConfig *config);
    static void RenderPrefsConfigToSerial();
    static void Close();
    static void SaveEverything();

    static void WebDebug(std::ostringstream *stream);

    // HTTPS Certificate management
    static bool LoadHTTPSCertificates(std::string &certPem, std::string &keyPem);
    static void SaveHTTPSCertificates(const std::string &certPem, const std::string &keyPem);

    static Preferences Handler;
    static uint32_t BootCount;
};

