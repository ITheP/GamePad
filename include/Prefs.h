#pragma once

#include <Preferences.h>
#include <Arduino.h>

class Prefs {
public:
    static void Begin();
    static void Save();
    static void Load();
    static void Close();
    static void SaveEverything();
    static Preferences Handler;
};
