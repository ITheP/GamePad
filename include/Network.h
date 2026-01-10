#pragma once

#include <WiFi.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <string>

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
    static int WiFiDisabled;
    static char *WiFiStatus;
    static unsigned char WiFiCharacter;

    static const char *ssid;
    static const char *password;

    static std::vector<AccessPoint*> AllAccessPointList;
    static std::map<String, AccessPoint*> AccessPointList;
    static int AccessPointListUpdated;

    static void Config_InitWifi();
    static void Config_StartScan();
    static void Config_UpdateScanResults();
    
    // WiFi connection testing
    enum WiFiTestResult {
        TEST_CONNECTING,      // Test is in progress
        TEST_SUCCESS,         // Successfully connected
        TEST_INVALID_PASSWORD, // Authentication failed
        TEST_SSID_NOT_FOUND,  // Network not available
        TEST_TIMEOUT,         // Connection attempt timed out
        TEST_FAILED,          // Other connection failure
        TEST_NOT_STARTED      // Test hasn't been initiated
    };
    
    static WiFiTestResult TestWiFiConnection(const String& testSSID, const String& testPassword);
    static WiFiTestResult CheckTestResults();  // Check and return current test status
    static String DescribeTestResults(String ssid, String password, WiFiTestResult testResults);
    static bool IsWiFiTestInProgress();
    static WiFiTestResult GetLastTestResult();
    static void CancelWiFiTest();  // Cancel any in-progress test

private:
    static int WiFiConnecting;
    static wl_status_t WiFiConnectionState;
    static wl_status_t PreviousWiFiConnectionState;

    static unsigned char WiFiStatusCharacter;
    static unsigned char LastWiFiCharacter;
    static unsigned char LastWiFiStatusCharacter;
    static unsigned char FinalWiFiCharacter;

    static int WiFiStatusIterations;
    
    // WiFi test state variables
    static WiFiTestResult LastTestResult;
    static bool TestInProgress;
    static unsigned long TestStartTime;
    static const unsigned long TEST_TIMEOUT_MS = 15000; // 15 second timeout for WiFi test

    static void RenderIcons();
};