#include <WiFi.h>
#include "Network.h"
#include "GamePad.h"
#include "Config.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "IconMappings.h"
#include "Icons.h"
#include "Web.h"
#include "UI.h"

#include "Secrets.h"

// TODO: If ndef WIFI then draw the icons for no wifi in main display setup

char WiFi_HighSignal[] = "OK - High Signal";
char WiFi_MediumSignal[] = "OK - Medium Signal";
char WiFi_LowSignal[] = "OK - Low Signal";
char WiFi_TraceSignal[] = "OK - Trace Signal";
char WiFi_Query[] = "Problem checking access point";
char WiFi_AccessPointUnavailable[] = "Access point not found";
char WiFi_Disabled[] = "Disabled";
char WiFi_Connecting[] = "Connecting...";
char WiFi_ReConnecting[] = "Reconnecting...";
char WiFi_Disconnected[] = "Disconnected";
char WiFi_UnknownStatus[] = "Unknown Status";

// Wi-Fi credentials
const char *Network::ssid = WIFI_SSID;
const char *Network::password = WIFI_PASSWORD;

int Network::WiFiConnecting = 0;
wl_status_t Network::WiFiConnectionState;
wl_status_t Network::PreviousWiFiConnectionState = WL_SCAN_COMPLETED; // Initialize to something we know it won't be to force a UI update straight away;

unsigned char Network::LastWiFiCharacter;
unsigned char Network::LastWiFiStatusCharacter;
int Network::WiFiStatusIterations;

// ToDo: Disconnected icon if wifi is turned off

unsigned char Network::WiFiCharacter;
char *Network::WiFiStatus;

unsigned char Network::WiFiStatusCharacter;
// We have a final character that might override the WiFiCharacter (e.g. animation) but we want to retain what
// the current WiFiCharacter is so that we can re-render it when status isn't changing. Mainly relevant when in a connecting state.
unsigned char Network::FinalWiFiCharacter;

void Network::HandleWiFi(int second)
{
    // WiFi
    // Connect to Wi-Fi
    WiFiConnectionState = WiFi.status();
    // FinalWiFiCharacter = WiFiCharacter;

    if (WiFiConnectionState == WL_CONNECTED)
    {
        wifi_ap_record_t ap_info;
        esp_err_t state = esp_wifi_sta_get_ap_info(&ap_info);

        // Just connected.
        if (PreviousWiFiConnectionState != WiFiConnectionState)
        {
#ifdef EXTRA_SERIAL_DEBUG
            Serial.println("Connected to WiFi...");

            if (state == ESP_OK)
            {
                Serial.println("Signal strength: " + String(ap_info.rssi) + " dBm");
                Serial.println("Channel: " + String(ap_info.primary));
            }

            // WiFi.setSleep(false);

            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());

            // esp_wifi_set_ps(WIFI_PS_NONE); // Throws wobbler if you try and use BT and WiFi at same time if WiFi isn't in a lower power mode

            wifi_ps_type_t ps_mode;
            esp_wifi_get_ps(&ps_mode);

            if (ps_mode == WIFI_PS_NONE)
            {
                Serial.println("Wi-Fi is not in any power-saving mode.");
            }
            else if (ps_mode == WIFI_PS_MIN_MODEM)
            {
                Serial.println("Wi-Fi is in minimum modem power-saving mode.");
            }
            else if (ps_mode == WIFI_PS_MAX_MODEM)
            {
                Serial.println("Wi-Fi is in maximum modem power-saving mode.");
            }
            else
            {
                Serial.println("Wi-Fi power saving mode is unknown.");
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
            // WiFiStatusCharacter = Icon_OK;
        }
        else
        {
#ifdef EXTRA_SERIAL_DEBUG
            Serial.println("ERROR: Bad state checking ap info: " + String(state));
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
        // WiFiStatusCharacter = (unsigned char)Icon_EyesLeft + (unsigned char)(WiFiStatusIterations & 1);

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
                    Serial.println("WIFI: Idle");
                    Serial.println("WIFI: Attempting to connect...");
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
                Serial.println("WIFI: Connection lost, attempting to reconnect...");
#endif
                WiFiCharacter = Icon_WiFi_LostSignal;
                WiFiStatus = WiFi_ReConnecting;
                WiFi.reconnect();
            }
            else if (WiFiConnectionState == WL_DISCONNECTED)
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("WIFI: Connection disconnected, attempting to reconnect...");
#endif

                WiFiCharacter = Icon_WiFi_LostSignal;
                WiFiStatus = WiFi_Disconnected;
                WiFi.reconnect();
            }
            else if (WiFiConnectionState == WL_NO_SSID_AVAIL)
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("WIFI: SSID [' + ssid + '] is not available or cannot be found");
#endif
                WiFiCharacter = Icon_WiFi_Query;
                WiFiStatus = WiFi_AccessPointUnavailable;
            }
            else
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("WIFI: Unknown status: " + String(WiFiConnectionState));
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
        // Serial.println((int)(animation));
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
    // if (WiFiStatusCharacter != LastWiFiStatusCharacter)
    //{
    //     RenderIcon(WiFiStatusCharacter, uiWiFiStatus_xPos, uiWiFiStatus_yPos, 16, 16);
    //     LastWiFiStatusCharacter = WiFiStatusCharacter;
    // }

    // TEMPORARY TEST
    Web::RenderIcons();
}