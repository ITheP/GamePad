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

// Wi-Fi credentials
const char *Network::ssid = WIFI_SSID;
const char *Network::password = WIFI_PASSWORD;

int Network::WifiConnecting = 0;
wl_status_t Network::WifiConnectionState;
wl_status_t Network::PreviousWifiConnectionState = WL_SCAN_COMPLETED; // Initialize to something we know it won't be to force a UI update straight away;

unsigned char Network::LastWifiCharacter;
unsigned char Network::LastWifiStatusCharacter;
int Network::WifiStatusIterations;

// ToDo: Disconnected icon if wifi is turned off

unsigned char Network::WifiCharacter;
unsigned char Network::WifiStatusCharacter;
// We have a final character that might override the WifiCharacter (e.g. animation) but we want to retain what
// the current WifiCharacter is so that we can re-render it when status isn't changing. Mainly relevant when in a connecting state.
unsigned char Network::FinalWifiCharacter;

void Network::HandleWifi(int second)
{
    // Wifi
    // Connect to Wi-Fi
    WifiConnectionState = WiFi.status();
    // FinalWifiCharacter = WifiCharacter;

    if (WifiConnectionState == WL_CONNECTED)
    {
        wifi_ap_record_t ap_info;
        esp_err_t state = esp_wifi_sta_get_ap_info(&ap_info);

        // Just connected.
        if (PreviousWifiConnectionState != WifiConnectionState)
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
            WifiConnecting = false;
        }

        // Wifi signal level can change at any time, so we constantly update it
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

            // Update Wifi icon with relevant signal level if required
            if (ap_info.rssi > -50)
                WifiCharacter = Icon_WIFI_HighSignal;
            else if (ap_info.rssi > -67)
                WifiCharacter = Icon_WIFI_MediumSignal;
            else if (ap_info.rssi > -75)
                WifiCharacter = Icon_WIFI_LowSignal;
            else
                WifiCharacter = Icon_WIFI_TraceSignal;

           //WifiStatusCharacter = Icon_OK;
        }
        else
        {
#ifdef EXTRA_SERIAL_DEBUG
            Serial.println("ERROR: Bad state checking ap info: " + String(state));
#endif

            // Somethings gone pretty wrong!
            if ((second & 1) == 0)
                WifiCharacter = Icon_WIFI_Query;
            else
                WifiCharacter = Icon_WIFI_Disabled;

            WifiStatusCharacter = Icon_Skull;
        }

        FinalWifiCharacter = WifiCharacter;
    }
    // else if (not enabled/disconnected state)
    // HERE TO DO
    else
    {
        // Eyes hunting icons
        //WifiStatusCharacter = (unsigned char)Icon_EyesLeft + (unsigned char)(WifiStatusIterations & 1);

        // Somethings changed...
        if (PreviousWifiConnectionState != WifiConnectionState)
        {
            WifiStatusIterations = 0;

            if (WifiConnectionState == WL_IDLE_STATUS || WifiConnectionState == WL_NO_SHIELD || WifiConnectionState == WL_CONNECT_FAILED)
            // WL_NO_SHIELD is Known to be an init state on device start up, so use it as an alternative to try and connect
            // WL_CONNECT_FAILED we just try and connect again
            {
                if (WifiConnecting == false)
                {
#ifdef EXTRA_SERIAL_DEBUG
                    Serial.println("WIFI: Idle");
                    Serial.println("WIFI: Attempting to connect...");
#endif
                    WiFi.disconnect(); // Reports that this helps if previously connected and disconnected

                    // WiFi.mode (WIFI_APSTA); //WIFI_STA);
                    // esp_wifi_set_ps(WIFI_PS_NONE);

                    WiFi.begin(ssid, password);
                    WifiConnecting = true;
                }

                WifiCharacter = Icon_WIFI_TraceSignal;
            }
            else if (WifiConnectionState == WL_CONNECTION_LOST)
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("WIFI: Connection lost, attempting to reconnect...");
#endif
                WifiCharacter = Icon_WIFI_LostSignal;
                WiFi.reconnect();
            }
            else if (WifiConnectionState == WL_DISCONNECTED)
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("WIFI: Connection disconnected, attempting to reconnect...");
#endif

                WifiCharacter = Icon_WIFI_LostSignal;
                WiFi.reconnect();
            }
            else if (WifiConnectionState == WL_NO_SSID_AVAIL)
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("WIFI: SSID [' + ssid + '] is not available or cannot be found");
#endif
                WifiCharacter = Icon_WIFI_Query;
            }
            else
            {
#ifdef EXTRA_SERIAL_DEBUG
                Serial.println("WIFI: Unknown status: " + String(WifiConnectionState));
#endif
                WifiCharacter = Icon_WIFI_Query;
            }
        }
        else
        {
            // We have a 0 based iteration counter, rather than using seconds
            // so that relevant iconography should always start from a 0 based offset
            WifiStatusIterations++;
        }

        // Base icon remains the same but we animate the search part of this
        unsigned char animation = (unsigned char)(WifiStatusIterations & 0b11);
        // Serial.println((int)(animation));
        if (animation == 0)
            FinalWifiCharacter = WifiCharacter;
        else
            FinalWifiCharacter = (unsigned char)Icon_WIFI_TraceSignal + animation; // 0-3

        Serial.print(animation);
        Serial.print(" - ");
        Serial.println((int)FinalWifiCharacter);
    }

    PreviousWifiConnectionState = WifiConnectionState;

    RenderIcons();
}

void Network::RenderIcons()
{
    if (FinalWifiCharacter != LastWifiCharacter)
    {
        RenderIcon(FinalWifiCharacter, uiWiFi_xPos, uiWiFi_yPos, 14, 16);
        LastWifiCharacter = FinalWifiCharacter;
    }
    //if (WifiStatusCharacter != LastWifiStatusCharacter)
    //{
    //    RenderIcon(WifiStatusCharacter, uiWiFiStatus_xPos, uiWiFiStatus_yPos, 16, 16);
    //    LastWifiStatusCharacter = WifiStatusCharacter;
    //}

    // TEMPORARY TEST
    Web::RenderIcons();
}