#include <WiFi.h>
#include "Network.h"
#include "GamePad.h"
#include "Config.h"
#include "esp_wifi.h"
#include "esp_log.h"

// TODO: If ndef WIFI then draw the icons for no wifi in main display setup

// Wi-Fi credentials
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

int WifiConnecting = 0;
wl_status_t WifiConnectionState;
wl_status_t PreviousWifiConnectionState = WL_SCAN_COMPLETED; // Initialize to something we know it won't be to force a UI update straight away;


void Network::HandleWifi(int second) {
    // Wifi
    // Connect to Wi-Fi
    WifiConnectionState = WiFi.status();

    if (WifiConnectionState == WL_CONNECTED)
    {
      wifi_ap_record_t ap_info;
      esp_err_t state = esp_wifi_sta_get_ap_info(&ap_info);

      if (PreviousWifiConnectionState != WifiConnectionState)
      {

        Serial.println("Connected to WiFi...");

        if (state == ESP_OK)
        {
          Serial.println("Signal strength: " + String(ap_info.rssi) + " dBm");
          Serial.println("Channel: " + String(ap_info.primary));
        }

        //WiFi.setSleep(false);

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

        WifiConnecting = 0;
      }

      if (state == ESP_OK)
      {
        // -30 to -50 dBm - Excellent signal
        // -50 to -67 dBm - Good
        // -67 to -75 dBm - Usable, occasional disconnects
        // -75 to -85 dBm - Weak, slow speeds or disconnects
        // < -85dBm       - Unusual, frequent disconnects
        Serial.println(ap_info.rssi);
      }
      // Update signal if required
      // TODO: SIGNAL STRENGTH ICON
    }
    else
    {
      if (PreviousWifiConnectionState != WifiConnectionState)
      {
        if (WifiConnectionState == WL_IDLE_STATUS || WifiConnectionState == WL_NO_SHIELD || WifiConnectionState == WL_CONNECT_FAILED)
        // WL_NO_SHIELD is Known to be an init state on device start up, so use it as an alternative to try and connect
        // WL_CONNECT_FAILED we just try and connect again
        {
          if (WifiConnecting == 0)
          {
            Serial.println("WIFI: Idle");
            Serial.println("WIFI: Attempting to connect...");
            WiFi.disconnect(); // Reports that this helps if previously connected and disconnected
            
            
            //WiFi.mode (WIFI_APSTA); //WIFI_STA);
            //esp_wifi_set_ps(WIFI_PS_NONE);
            
            
            WiFi.begin(ssid, password);
            WifiConnecting = 1;
          }
        }
        else if (WifiConnectionState == WL_CONNECTION_LOST)
        {
          Serial.println("WIFI: Connection lost, attempting to reconnect...");

          WiFi.reconnect();
        }
        // else if (WifiConnectionState == WL_CONNECT_FAILED)
        // {
        //   Serial.println("WIFI: Connection failed");
        // }
        else if (WifiConnectionState == WL_DISCONNECTED)
        {
          Serial.println("WIFI: Connection disconnected, attempting to reconnect...");

          WiFi.reconnect();
        }
        else if (WifiConnectionState == WL_NO_SSID_AVAIL)
        {
          Serial.println("WIFI: SSID [' + ssid + '] is not available or cannot be found");
        }
        else
        {
          Serial.println("WIFI: Unknown status: " + String(WifiConnectionState));
        }
      }

      if (WifiConnecting > 0)
      {
        // TODO: Connecting ICON

        WifiConnecting++;
      }
    }

  PreviousWifiConnectionState = WifiConnectionState;
}