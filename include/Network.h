#pragma once

#include <WiFi.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <string>

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


// Structure to hold AP info
struct AccessPoint {
  String ssid;        // Network name
  String bssid;       // MAC address reference
  int32_t rssi;       // Signal strength
  wifi_auth_mode_t authMode; // Encryption type
  bool selected;      // Preferred flag

  unsigned char WiFiCharacter;
  char *WiFiStatus;
};

class Network
{
public:
    static void HandleWiFi(int second);
    static char *WiFiStatus;

    static std::vector<AccessPoint*> AllAccessPointList;
    static std::map<String, AccessPoint*> AccessPointList;
    static int AccessPointListUpdated;

    static void Config_InitiWifi();
    static void Config_StartScan();
    static void Config_UpdateScanResults();
    static void Config_SelectAccessPoint(const String& ssid);

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