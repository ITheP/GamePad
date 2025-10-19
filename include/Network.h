#pragma once

#include <WiFi.h>
#include <stdint.h>

// enum WiFi_StatusCode {
//     WIFI_HIGHSIGNAL,
//     WIFI_MEDIUMSIGNAL,
//     WIFI_LOWSIGNAL,
//     WIFI_TRACESIGNAL,
//     WIFI_QUERY,
//     WIFI_ACCESSPOINTUNAVAILABLE,
//     WIFI_DISABLED,
//     WIFI_CONNECTING,
//     WIFI_RECONNECTING,
//     WIFI_DISCONNECTED,
//     WIFI_UNKNOWNSTATUS,
//     WIFI_STATUS_COUNT // for bounds checking
// };

class Network
{
public:
    static void HandleWiFi(int second);

    static char *WiFiStatus;

    // static WiFi_StatusCode WiFiStatusCode;
    // static constexpr char* WiFiStatusText[WIFI_STATUS_COUNT] = {
    // "OK - High Signal",
    // "OK - Medium Signal",
    // "OK - Low Signal",
    // "OK - Trace Signal",
    // "Problem checking access point",
    // "Access point not found",
    // "Disabled",
    // "Connecting...",
    // "Reconnecting...",
    // "Disconnected",
    // "Unknown Status"
    // };

private:
    static const char *ssid;
    static const char *password;

    static int WiFiConnecting;
    static wl_status_t WiFiConnectionState;
    static wl_status_t PreviousWiFiConnectionState;

    static unsigned char WiFiCharacter;
    static unsigned char WiFiStatusCharacter;
    static unsigned char LastWiFiCharacter;
    static unsigned char LastWiFiStatusCharacter;
    static unsigned char FinalWiFiCharacter;

    static int WiFiStatusIterations;

    static void RenderIcons();
};