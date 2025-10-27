#pragma once

#include <Preferences.h>
#include <Arduino.h>

class Prefs {
public:
    static void Init();
    static void Save();
    static void Load();
    static void Close();
    static void SaveEverything();

    static void WebDebug(std::ostringstream *stream);

    static Preferences Handler;
    static uint32_t BootCount;
};
