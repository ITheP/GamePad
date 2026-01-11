#include <WiFi.h>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include "Network.h"
#include "GamePad.h"
#include "Config.h"
#include "Defines.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "IconMappings.h"
#include "Icons.h"
#include "Web.h"
#include "UI.h"
#include "debug.h"
#include "Defaults.h"

#include "Secrets.h"
#include <Utils.h>

char WiFi_HighSignal[] = "WiFi OK - High Signal";
char WiFi_MediumSignal[] = "WiFi OK - Medium Signal";
char WiFi_LowSignal[] = "WiFi OK - Low Signal";
char WiFi_TraceSignal[] = "WiFi OK - Trace Signal";
char WiFi_Query[] = "WiFi Problem checking access point";
char WiFi_AccessPointUnavailable[] = "WiFi Access point not found";
char WiFi_Disabled[] = "WiFi Disabled";
char WiFi_Connecting[] = "WiFi Connecting...";
char WiFi_ReConnecting[] = "WiFi Reconnecting...";
char WiFi_Disconnected[] = "WiFi Disconnected";
char WiFi_UnknownStatus[] = "WiFi Unknown Status";

char WiFi_SignalLevel_High[] = "High Signal";
char WiFi_SignalLevel_Medium[] = "Medium Signal";
char WiFi_SignalLevel_Low[] = "Low Signal";
char WiFi_SignalLevel_Trace[] = "Trace Signal";

// Dynamic list of APs
std::vector<AccessPoint *> Network::AllAccessPointList;
std::map<String, AccessPoint *> Network::AccessPointList;
int Network::AccessPointListUpdated = false;
int Network::WiFiDisabled = true;

const char *AuthModeToStr(wifi_auth_mode_t auth)
{
    switch (auth)
    {
    case WIFI_AUTH_OPEN:
        return "Open";
    case WIFI_AUTH_WEP:
        return "WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA/WPA2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "WPA2-Enterprise";
    case WIFI_AUTH_WPA3_PSK:
        return "WPA3-PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WPA2/WPA3-PSK";
    case WIFI_AUTH_WAPI_PSK:
        return "WAPI-PSK";
    default:
        return "Unknown";
    }
}

// Note: Arduino only supports scanning with the results returned
// when the scan is complete, so we can't get partial results as it's happening
void Network::Config_StartScan()
{
#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🛜 Starting async WiFi scan...");
#endif

    WiFi.scanNetworks(true, true); // async=true, hidden=true
}

void Network::Config_UpdateScanResults()
{
    int scanStatus = WiFi.scanComplete();

    if (scanStatus == WIFI_SCAN_RUNNING)
    {
        // Still scanning
        // Serial.println("WiFi scan still in progress...");
        return;
    }

    if (scanStatus >= 0)
    {
        // Scan finished, results available
        // Clean up old entries first
        for (auto *ap : AllAccessPointList)
            delete ap;

        AllAccessPointList.clear();

        for (int i = 0; i < scanStatus; ++i)
        {
            AccessPoint *ap = new AccessPoint();

            ap->ssid = WiFi.SSID(i);
            ap->bssid = WiFi.BSSIDstr(i);
            ap->rssi = WiFi.RSSI(i);
            ap->authMode = WiFi.encryptionType(i);
            ap->selected = false;

            if (ap->rssi > -50)
            {
                ap->WiFiCharacter = Icon_WiFi_HighSignal;
                ap->WiFiStatus = WiFi_SignalLevel_High;
            }
            else if (ap->rssi > -67)
            {
                ap->WiFiCharacter = Icon_WiFi_MediumSignal;
                ap->WiFiStatus = WiFi_SignalLevel_Medium;
            }
            else if (ap->rssi > -75)
            {
                ap->WiFiCharacter = Icon_WiFi_LowSignal;
                ap->WiFiStatus = WiFi_SignalLevel_Low;
            }
            else
            {
                ap->WiFiCharacter = Icon_WiFi_TraceSignal;
                ap->WiFiStatus = WiFi_SignalLevel_Trace;
            }

            AllAccessPointList.push_back(ap);
        }

#ifdef EXTRA_SERIAL_DEBUG
        Serial_INFO;
        Serial.printf("🛜 ℹ️ Async scan complete: %d networks found\n", scanStatus);
#endif

        // Generate final list of access points, removing duplicates (keeping strongest signal only)
        // and ignoring hidden SSIDs

        // Clean up old AccessPointList entries before clearing
        AccessPointList.clear();

        for (auto *ap : AllAccessPointList)
        {
            if (!ap || ap->ssid.length() == 0)
                continue; // skip hidden

            auto it = AccessPointList.find(ap->ssid);
            if (it == AccessPointList.end() || ap->rssi > it->second->rssi)
            {
                AccessPointList[ap->ssid] = ap; // keep strongest
            }
        }

        // Show all access points

        // // Sort by signal strength
        // std::sort(AccessPointList.begin(), uniqueAPs.end(),
        //           [](const AccessPoint &a, const AccessPoint &b)
        //           {
        //               return a.rssi > b.rssi; // higher RSSI (closer to 0) comes first
        //           });

        // // All access points
        // for (size_t i = 0; i < AllAccessPointList.size(); ++i)
        // {
        //     // Skip any with empty SSID (no name)
        //     // if (AccessPointList[i].ssid.length() == 0) {
        //     //     continue;
        //     // }

        //     Serial.printf("[%d] SSID: %s | BSSID: %s | RSSI: %d dBm | Auth: %d | Selected: %s\n",
        //                   (int)i,
        //                   AllAccessPointList[i].ssid.c_str(),
        //                   AllAccessPointList[i].bssid.c_str(),
        //                   AllAccessPointList[i].rssi,
        //                   AllAccessPointList[i].authMode,
        //                   AllAccessPointList[i].selected ? "YES" : "NO");
        // }

        // Show optimised list of access points, unique and strongest signals only

#ifdef EXTRA_SERIAL_DEBUG
        Serial_INFO;
        Serial.printf("🛜 %d network entries usable after removing duplicates and hidden entries\n", AccessPointList.size());
#endif

        // for (auto &entry : AccessPointList)
        // {
        //     const String &ssid = entry.first;     // the key
        //     AccessPoint *ap = entry.second; // the value

        //     // Skip hidden/empty SSIDs
        //     if (ssid.length() == 0)
        //         continue;

        //     Serial.printf("SSID: %s | BSSID: %s | RSSI: %s (%d dBm) | Auth: %s (%d) | Selected: %s\n",
        //                   ssid.c_str(),
        //                   ap->bssid.c_str(),
        //                   ap->WiFiStatus,
        //                   ap->rssi,
        //                   AuthModeToStr(ap->authMode),
        //                   ap->authMode,
        //                   ap->selected ? "YES" : "NO");
        // }

        AccessPointListUpdated = true;

        // Explicitly delete WiFi scan results to free the library's internal buffer
        WiFi.scanDelete();

        // Start next scan automatically (with interval enforcement)
        Config_StartScan();
    }
}

void Network::Config_InitWifi()
{
#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🛜 Config: Initializing WiFi");
#endif

    // Serial.begin(115200);
    //WiFi.mode(WIFI_STA);
    WiFi.disconnect(true); // Make sure no previous connections remain

// delay(1000);
#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.println("🛜 Config: Starting WiFi scanning");
#endif

    Config_StartScan();
}

// Wi-Fi credentials
const char *Network::ssid;
const char *Network::password;

int Network::WiFiConnecting = 0;
wl_status_t Network::WiFiConnectionState;
wl_status_t Network::PreviousWiFiConnectionState = WL_SCAN_COMPLETED; // Initialize to something we know it won't be to force a UI update straight away;

unsigned char Network::LastWiFiCharacter;
unsigned char Network::LastWiFiStatusCharacter;
int Network::WiFiStatusIterations;

// WiFi test state
Network::WiFiTestResult Network::LastTestResult = Network::TEST_NOT_STARTED;
bool Network::TestInProgress = false;
unsigned long Network::TestStartTime = 0;

unsigned char Network::WiFiCharacter;
char *Network::WiFiStatus;

unsigned char Network::WiFiStatusCharacter;

// We have a final character that might override the WiFiCharacter (e.g. something animated) but we want to retain what
// the current WiFiCharacter is so that we can re-render it when status isn't changing.
// Mainly relevant when in a connecting state.
unsigned char Network::FinalWiFiCharacter;

void Network::HandleWiFi(int second)
{
    if (WiFiDisabled)
    {
        LastWiFiCharacter = WiFiCharacter;
        RenderIcons();
        return;
    }

    WiFiConnectionState = WiFi.status();

    if (WiFiConnectionState == WL_CONNECTED)
    {
        wifi_ap_record_t ap_info;
        esp_err_t state = esp_wifi_sta_get_ap_info(&ap_info);

        // Just connected.
        if (PreviousWiFiConnectionState != WiFiConnectionState)
        {
#ifdef EXTRA_SERIAL_DEBUG
            Serial_INFO;
            Serial.println("🛜 ℹ️ WiFi Information...");

            if (state == ESP_OK)
            {
                Serial.println("🛜 📶 Signal strength: " + String(ap_info.rssi) + " dBm");
                Serial.println("🛜 📶 Channel: " + String(ap_info.primary));
            }

            // WiFi.setSleep(false);

            Serial.print("🛜 🔢 IP Address: ");
            Serial.println(WiFi.localIP());

            // esp_wifi_set_ps(WIFI_PS_NONE); // Throws wobbler if you try and use BT and WiFi at same time if WiFi isn't in a lower power mode

            wifi_ps_type_t ps_mode;
            esp_wifi_get_ps(&ps_mode);

            if (ps_mode == WIFI_PS_NONE)
            {
                Serial.println("🛜 ℹ️ WiFi is not in any power-saving mode.");
            }
            else if (ps_mode == WIFI_PS_MIN_MODEM)
            {
                Serial.println("🛜 ℹ️ WiFi is in minimum modem power-saving mode.");
            }
            else if (ps_mode == WIFI_PS_MAX_MODEM)
            {
                Serial.println("🛜 ℹ️ WiFi is in maximum modem power-saving mode.");
            }
            else
            {
                Serial.println("🛜 ⚠️ WiFi power saving mode is unknown.");
            }
#endif
            WiFiConnecting = false;
        }

        // WiFi signal level can change at any time, so we constantly update it
        if (state == ESP_OK)
        {
            // -30 to -50 dBm - Excellent signal
            // -50 to -67 dBm - Good
            // -67 to -75 dBm - Usable, occasional disconnects
            // -75 to -85 dBm - Weak, slow speeds or disconnects
            // < -85dBm       - Unusual, frequent disconnects

#ifdef EXTRA_SERIAL_DEBUG_PLUS
            Serial.println("WiFi Strength: " + String(ap_info.rssi));
#endif

            // Update WiFi icon with relevant signal level if required
            if (ap_info.rssi > -50)
            {
                WiFiCharacter = Icon_WiFi_HighSignal;
                WiFiStatus = WiFi_HighSignal;
            }
            else if (ap_info.rssi > -67)
            {
                WiFiCharacter = Icon_WiFi_MediumSignal;
                WiFiStatus = WiFi_MediumSignal;
            }
            else if (ap_info.rssi > -75)
            {
                WiFiCharacter = Icon_WiFi_LowSignal;
                WiFiStatus = WiFi_LowSignal;
            }
            else
            {
                WiFiCharacter = Icon_WiFi_TraceSignal;
                WiFiStatus = WiFi_TraceSignal;
            }
        }
        else
        {

#ifdef EXTRA_SERIAL_DEBUG
            Serial_ERROR;
            Serial.println("🛜 ❌ ERROR: Bad state checking Access Point info: " + String(state));
#endif

            // Somethings gone pretty wrong!
            if ((second & 1) == 0)
            {
                WiFiCharacter = Icon_WiFi_Query;
                WiFiStatus = WiFi_Query;
            }
            else
            {
                WiFiCharacter = Icon_WiFi_Disabled;
                WiFiStatus = WiFi_Disabled;
            }

            WiFiStatusCharacter = Icon_Skull;
        }

        FinalWiFiCharacter = WiFiCharacter;
    }
    // else if (not enabled/disconnected state)
    // HERE TO DO
    else
    {
        // Eyes hunting icons

        // Somethings changed...
        if (PreviousWiFiConnectionState != WiFiConnectionState)
        {
            WiFiStatusIterations = 0;

            if (WiFiConnectionState == WL_IDLE_STATUS || WiFiConnectionState == WL_NO_SHIELD || WiFiConnectionState == WL_CONNECT_FAILED)
            // WL_NO_SHIELD is Known to be an init state on device start up, so use it as an alternative to try and connect
            // WL_CONNECT_FAILED we just try and connect again
            {
                if (WiFiConnecting == false)
                {
#ifdef EXTRA_SERIAL_DEBUG
                    Serial.println("🛜 ⏳ WIFI: Attempting to connect...");
#endif
                    WiFi.disconnect(); // Reports that this helps if previously connected and disconnected

                    // WiFi.mode (WIFI_APSTA); //WIFI_STA);
                    // esp_wifi_set_ps(WIFI_PS_NONE);

                    WiFi.begin(ssid, password);
                    WiFiConnecting = true;
                    WiFiStatus = WiFi_Connecting;
                }

                WiFiCharacter = Icon_WiFi_TraceSignal;
            }
            else if (WiFiConnectionState == WL_CONNECTION_LOST)
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("🛜 ❌ WIFI: Connection lost, attempting to reconnect...");
#endif
                WiFiCharacter = Icon_WiFi_LostSignal;
                WiFiStatus = WiFi_ReConnecting;
                WiFi.reconnect();
            }
            else if (WiFiConnectionState == WL_DISCONNECTED)
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("🛜 ❌ WIFI: Connection disconnected, attempting to reconnect...");
#endif

                WiFiCharacter = Icon_WiFi_LostSignal;
                WiFiStatus = WiFi_Disconnected;
                WiFi.reconnect();
            }
            else if (WiFiConnectionState == WL_NO_SSID_AVAIL)
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("🛜 ⛔ WIFI: SSID [' + ssid + '] is not available or cannot be found");
#endif
                WiFiCharacter = Icon_WiFi_Query;
                WiFiStatus = WiFi_AccessPointUnavailable;
            }
            else
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("🛜 ❌ WIFI: Unknown status: " + String(WiFiConnectionState));
#endif
                WiFiCharacter = Icon_WiFi_Query;
                WiFiStatus = WiFi_UnknownStatus;
            }
        }
        else
        {
            // We have a 0 based iteration counter, rather than using seconds
            // so that relevant iconography should always start from a 0 based offset
            WiFiStatusIterations++;
        }

        // Base icon remains the same but we animate the search part of this
        unsigned char animation = (unsigned char)(WiFiStatusIterations & 0b11);

        if (animation == 0)
            FinalWiFiCharacter = WiFiCharacter;
        else
            FinalWiFiCharacter = (unsigned char)Icon_WiFi_TraceSignal + animation; // 0-3
    }

    PreviousWiFiConnectionState = WiFiConnectionState;

    RenderIcons();
}

void Network::RenderIcons()
{
    if (FinalWiFiCharacter != LastWiFiCharacter)
    {
        RenderIcon(FinalWiFiCharacter, uiWiFi_xPos, uiWiFi_yPos, 14, 16);
        LastWiFiCharacter = FinalWiFiCharacter;
    }

    // TEMPORARY TEST
    Web::RenderIcons();
}

// WiFi Connection Testing Functions
Network::WiFiTestResult Network::TestWiFiConnection(const String &testSSID, const String &testPassword)
{
    // Cancel any existing test first
    if (TestInProgress || TestStartTime > 0)
    {
        Serial_INFO;
        Serial.println("🛜 WiFi Test: Cancelling previous test");
        CancelWiFiTest();
    }

    Serial_INFO;
    Serial.printf("🛜 ❔ WiFi testing connection: SSID='%s', Password=%s\n", testSSID.c_str(), SaferPasswordString(testPassword).c_str());
    Serial_INFO;


    TestInProgress = true;
    TestStartTime = millis();
    LastTestResult = TEST_CONNECTING;

    // Disconnect from any existing connection first
    WiFi.disconnect(true);

    // delay(100);

    // Set to station mode and initiate connection
    WiFi.mode(WIFI_AP_STA); // Mixed mode when testing, might be needed!
    WiFi.begin(testSSID.c_str(), testPassword.c_str());

    return TEST_CONNECTING;
}

// Check the status of the current WiFi test and return results
Network::WiFiTestResult Network::CheckTestResults()
{
    // If no test is in progress, return the last result
    if (!TestInProgress)
    {
        return LastTestResult;
    }

    // Check if test has timed out
    if (TestStartTime > 0 && (millis() - TestStartTime) > TEST_TIMEOUT_MS)
    {
        TestInProgress = false;
        LastTestResult = TEST_TIMEOUT;
        TestStartTime = 0;

        Serial_INFO;
        Serial.println("🛜 ❌ WiFi Test: TIMEOUT - Connection attempt took too long");

        // Clean up the test connection
        WiFi.disconnect(true);

        return LastTestResult;
    }

    // Check connection status
    wl_status_t status = WiFi.status();

    if (status == WL_CONNECTED)
    {
        TestInProgress = false;
        LastTestResult = TEST_SUCCESS;
        TestStartTime = 0;

        Serial_INFO;
        Serial.printf("🛜 ✅ WiFi Test: SUCCESS - Connected with IP: %s\n", WiFi.localIP().toString().c_str());

        return TEST_SUCCESS;
    }
    else if (status == WL_CONNECT_FAILED)
    {
        TestInProgress = false;
        LastTestResult = TEST_INVALID_PASSWORD;
        TestStartTime = 0;

        Serial_INFO;
        Serial.println("🛜 ⚠️ WiFi Test: FAILED - Connection attempt failed (likely invalid password)");
        WiFi.disconnect(true);

        return TEST_INVALID_PASSWORD;
    }
    else if (status == WL_NO_SSID_AVAIL)
    {
        TestInProgress = false;
        LastTestResult = TEST_SSID_NOT_FOUND;
        TestStartTime = 0;

        Serial_INFO;
        Serial.println("🛜 ❌ WiFi Test: FAILED - SSID not found/available");
        WiFi.disconnect(true);

        return TEST_SSID_NOT_FOUND;
    }

    // Still connecting...
    return TEST_CONNECTING;
}

bool Network::IsWiFiTestInProgress()
{
    return TestInProgress || (TestStartTime > 0 && (millis() - TestStartTime) <= TEST_TIMEOUT_MS);
}

Network::WiFiTestResult Network::GetLastTestResult()
{
    return LastTestResult;
}

void Network::CancelWiFiTest()
{
    if (TestInProgress || TestStartTime > 0)
    {
        Serial_INFO;
        Serial.println("🛜 ℹ️ WiFi Test: Cancelled");

        TestInProgress = false;
        TestStartTime = 0;
        LastTestResult = TEST_NOT_STARTED;

        // Clean up the test connection
        WiFi.disconnect(true);
    }
}

String Network::DescribeTestResults(String ssid, String password, WiFiTestResult testResults)
{
    String resultText;

    // Note we only return no password set if not successful (incase doesn't need a password)

    if (ssid.length() == 0)
        resultText = "Access Point not set";
    else if (testResults == Network::TEST_NOT_STARTED)
        resultText = "Pending Test!";
    else if (testResults == Network::TEST_SUCCESS)
        resultText = "Success!";
    else if (testResults == Network::TEST_SSID_NOT_FOUND)
        resultText = "SSID Not Found";
    else if (testResults == Network::TEST_CONNECTING)
        resultText = "Testing...";
    else if (password.length() == 0)
    {
        switch (testResults)
        {
        case Network::TEST_INVALID_PASSWORD:
            resultText = "Password missing";
            break;
        case Network::TEST_TIMEOUT:
            resultText = "Timeout, missing password?";
            break;
        case Network::TEST_FAILED:
            resultText = "Couldn't connect, missing password?";
            break;
        default:
            resultText = "Unknown";
        }
    }
    else
    {
        switch (testResults)
        {
        case Network::TEST_INVALID_PASSWORD:
            resultText = "Invalid Password";
            break;
        case Network::TEST_SSID_NOT_FOUND:
            resultText = "SSID Not Found";
            break;
        case Network::TEST_TIMEOUT:
            resultText = "Timeout";
            break;
        case Network::TEST_FAILED:
            resultText = "Couldn't connect";
            break;
        default:
            resultText = "Unknown";
        }
    }

    return resultText;
}