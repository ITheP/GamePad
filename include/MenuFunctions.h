#pragma once

#include "config.h"
#include "Profiles.h"

#define MenuContentStartX 22
#define MenuContentStartY 20

class MenuFunctions
{
public:
    static Profile *Current_Profile;
    static int Profile_Selected;

    // Init... - functions called when entering menu option (e.g. draw static elements)
    // Update... - functions called repeatedly while in menu option (e.g. update changing elements)
    // Draw... - functions to draw specific elements that might be used across the above

    static void Setup();

    // Generally speaking there are 2-3 functions for each menu function, but it isn't compulsory
    // Init...() generally draws initial menu information, or initialises anything that menu function needs
    // Update...() is called each display frame, so can do things like animate a menu function or keep display updated with changing values
    // Draw...() is sometimes used as a helper function that Init...() and Update...() can use to draw core content of that menu with shared code

    // Config Menu Functions
    static void Config_Setup();
    static void Config_Init_Help();
    static void Config_Update_Help();
    static void Config_Draw_Help(int showScrollIcons = false);

    static void Config_Init_Profile();
    static void Config_Update_Profile();
    static void Config_Draw_Profile(int showScrollIcons = false);

    // static void Config_Init_WiFi_Settings();
    // static void Config_Update_WiFi_Settings();
    // static void Config_Draw_WiFi_Settings();

    static void Config_Init_WiFi_AccessPoint();
    static void Config_Update_WiFi_AccessPoint();
    static void Config_Draw_WiFi_AccessPoint(int showScrollIcons = false);

    static void Config_Init_WiFi_Password();
    static void Config_Update_WiFi_Password();
    static void Config_Draw_WiFi_Password(int showScrollIcons = false);
    static void Config_Exit_WiFi_Password();

    static void Config_Init_SaveSettings();
    static void Config_Update_SaveSettings();
    static void Config_Draw_SaveSettings();

    // ToDo: ClearProfile()
    // ToDo: CloneProfile() (mark a profile, then copy marked to current)

    static void DrawScrollArrows();

    // Main Menu Functions

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

private:
    static char Icon;
    static int AnimationFrameIndex; // Index for tracking animated components
    static int FrameTimer;
    static int DelayBetweenFrames; // milliseconds between animation frames
    static String MessageTop;          // General purpose message
    static String MessageBottom;          // General purpose message

    static void DrawMessages();
};