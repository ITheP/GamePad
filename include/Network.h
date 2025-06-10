#pragma once

#include <WiFi.h>
#include <stdint.h>

class Network
{
public:
    static void HandleWifi(int second);

private:
    static const char *ssid;
    static const char *password;

    static int WifiConnecting;
    static wl_status_t WifiConnectionState;
    static wl_status_t PreviousWifiConnectionState;

    static unsigned char WifiCharacter;
    static unsigned char WifiStatusCharacter;
    static unsigned char LastWifiCharacter;
    static unsigned char LastWifiStatusCharacter;
    static unsigned char FinalWifiCharacter;

    static int WifiStatusIterations;

    static void RenderIcons();
};