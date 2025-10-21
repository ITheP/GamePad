#pragma once

#include "config.h"

class MenuFunctions
{
public:
// Init... - functions called when entering menu option (e.g. draw static elements)
    // Update... - functions called repeatedly while in menu option (e.g. update changing elements)
    // Draw... - functions to draw specific elements that might be used across the above

    static void Setup();

    static void InitName();

    static void InitAbout();

    static void InitVersion();

    static void InitBattery();
    static void UpdateBattery();
    static void DrawBatteryLevel();

    static void InitWiFi();
    static void UpdateWiFi();

    static void InitBluetooth();
    static void UpdateBluetooth();
    static void DrawBluetooth();

    static void InitWebServer();
    static void UpdateWebServer();
    static void DrawWebServer();

    static void InitFPS();
    static void UpdateFPS();
    static void DrawFPS();

    static void InitUSB();
    static void UpdateUSB();
    static void DrawUSB();

    static void InitStats();

    static void InitDebug();
};