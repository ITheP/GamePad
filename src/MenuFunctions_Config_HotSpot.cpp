#include "Menu.h"
#include "Menus.h"
#include "MenuFunctions.h"
#include "Config.h"
#include "Defines.h"
#include "Structs.h"
#include "IconMappings.h"
#include "Screen.h"
#include "RenderText.h"
#include "Icons.h"
#include "DeviceConfig.h"
#include "Utils.h"
#include "GamePad.h"
#include "Battery.h"
#include "Network.h"
#include "Web.h"
#include "Debug.h"
#include <Profiles.h>
#include <Arduino.h>

// On open...
// Start hot spot so people can connect via web site
// Display info about hotspot (IP Address)
// Display current profiles ssid + password
// Display current ssid (if any) and password as per password input screen
// Remember current ssid and password
// in the update...
// If either ssid or password changes, cancel any existing test and start a new one, as per password input screen

static String HotspotName;
static String HotspotIP;
static String CurrentPassword = ""; // Track last tested password to know when to test again
static String CurrentSSID = "";     // Track last tested SSID to know when to test again
static bool credentialsChanged = false;
static Network::WiFiTestResult WiFiTestResult = Network::TEST_NOT_STARTED;

void MenuFunctions::Config_Init_Hotspot()
{
  Menus::InitMenuItemDisplay(true);

  WiFiTestResult = Network::TEST_NOT_STARTED;

  HotspotName = String(ControllerType);

#ifdef EXTRA_SERIAL_DEBUG
  Serial_INFO;
  Serial.println("🛑 Initialising Hotspot WiFi Access Point '" + HotspotName + "'");
#endif

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(HotspotName, "12345678", 1, false);
  IPAddress apIP = WiFi.softAPIP();

#ifdef EXTRA_SERIAL_DEBUG
  if (WiFi.softAPgetStationNum() >= 0)
    Serial.println("🛑 ✅ SoftAP is active");
  else
    Serial.println("🛑 ❌ SoftAP failed to start");

  Serial.print("🛑 SoftAP SSID: ");
  Serial.println(WiFi.softAPSSID());

  Serial.print("🛑 SoftAP IP: ");
  Serial.println(apIP);


    Serial_INFO;
    Serial.printf("🛑 Profile selected: %d - %s, WiFi: %s, WiFi Password: %s\n", CurrentProfile->Id, CurrentProfile->Description.c_str(), CurrentProfile->WiFi_Name.c_str(), SaferPasswordString(CurrentProfile->WiFi_Password).c_str());
  #endif

  Web::StartServer();

  // while (true) {
  //   Serial.println(String(Second));
  //   delay(1000);
  // }

  CurrentSSID = "";
  CurrentPassword = "";
  // force processing of changed credentials on first run
  credentialsChanged = true;

  Config_Update_Hotspot();
  Config_Draw_Hotspot();
}

void MenuFunctions::Config_Update_Hotspot()
{
  Menus::Config_CheckForMenuChange();

  if (CurrentSSID != CurrentProfile->WiFi_Name || CurrentPassword != CurrentProfile->WiFi_Password)
  {
    credentialsChanged = true;
    CurrentSSID = CurrentProfile->WiFi_Name;
    CurrentPassword = CurrentProfile->WiFi_Password;
  }

  if (credentialsChanged)
  {
    credentialsChanged = false;

    // Cancel any in-progress test
    if (Network::IsWiFiTestInProgress())
      Network::CancelWiFiTest();

    // Start a new WiFi test if SSID and password are available
    if (CurrentSSID.length() > 0)
    {
      // Start a new WiFi test
      Network::TestWiFiConnection(CurrentSSID, CurrentPassword);

#ifdef EXTRA_SERIAL_DEBUG
      Serial_INFO;
      Serial.printf("WiFi credentials changed - Hotspot testing connection with SSID: '%s'\n", CurrentSSID.c_str());
#endif
    }
  }
  if (DisplayRollover)
    MenuFunctions::Config_Draw_Hotspot();

  // static int test = 1;
  // if (DisplayRollover)
  // {
  //   if (test % 300 == 0)
  //   {

  //     auto aps = Network::AccessPointList;

  //     Serial.println();
  //     Serial_INFO;
  //     Serial.println("🛜 📶 Current Access Points:");
  //     for (auto &entry : aps)
  //     {

  //       AccessPoint *ap = entry.second; // the value

  //       if (!ap || ap->ssid.length() == 0)
  //         continue; // skip hidden

  //       Serial.print("SSID: ");
  //       Serial.print(ap->ssid);
  //       Serial.print(", RSSI: ");
  //       Serial.println(ap->rssi);
  //     }
  //     Serial.println();
  //   }

  //   test++;
  // }
}

void MenuFunctions::Config_Draw_Hotspot()
{
  Display.fillRect(0, MenuContentStartY, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY), C_BLACK);

  ResetPrintDisplayLine(MenuContentStartY, 0, SetFontSmall);

  sprintf(buffer, "Connect to WiFi %s", HotspotName.c_str());
  PrintDisplayLineCentered(buffer);
  sprintf(buffer, "Browse %s", WiFi.softAPIP().toString().c_str());
  PrintDisplayLineCentered(buffer);

  WiFiTestResult = Network::CheckTestResults();

  String resultText = Network::DescribeTestResults(CurrentSSID, CurrentPassword, WiFiTestResult);

  sprintf(buffer, "Status: %s", resultText.c_str());
  PrintDisplayLineCentered(buffer);

  // Draw ssid + password at the bottom of the screen
  // TODO: Shrink password
  sprintf(buffer, "%s / %s", CurrentSSID.length() > 0 ? CurrentSSID.c_str() : "<not set>", CurrentPassword.length() > 0 ? SaferShortenedPasswordString(CurrentPassword).c_str() : "<not set>");
  if (RREDefault.strWidth(buffer) > SCREEN_WIDTH)
    SetFontSmall();
  else
    SetFontFixed();

  RRE.printStr(0, SCREEN_HEIGHT - TextLineHeight, buffer);
}

void MenuFunctions::Config_Exit_Hotspot()
{
  // Cancel any in-progress WiFi test when exiting the menu
  if (Network::IsWiFiTestInProgress())
  {
    Network::CancelWiFiTest();
  }

  Web::StopServer();

  WiFi.disconnect(true);

  // Just in case
  Network::Config_InitWifi();
}