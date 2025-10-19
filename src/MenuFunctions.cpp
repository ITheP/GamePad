#include "Menu.h"
#include "Menus.h"
#include "MenuFunctions.h"
#include "Config.h"
#include "Structs.h"
#include "IconMappings.h"
#include "Screen.h"
#include "RenderText.h"
#include "Icons.h"
#include "DeviceConfig.h"
#include "Utils.h"
#include <GamePad.h>
#include <Battery.h>
#include <Network.h>

extern CRGB ExternalLeds[];

#define MenuContentStartX 22

// char AboutDetails[128];
// int AboutDetailsLength = 0;
// int AboutDetailsCopyLength = 0;
// int aboutCharPos = 0;
// int startCharWidth;
// int aboutPixelPos = startCharWidth;

void MenuFunctions::Setup()
{
  // AboutDetailsLength = snprintf(AboutDetails, sizeof(AboutDetails), "%s - Core %s - FW v%s - HW v%s - SW v%s - ",
  //                               ModelNumber, getBuildVersion(), FirmwareRevision, HardwareRevision, SoftwareRevision);

  // // Copy start of AboutDetails to end of itself so when scrolling we can overrun the end without having to wrap to the start
  // AboutDetailsCopyLength = (SCREEN_WIDTH / 8) + 1; // Number of chars that fit on screen + 1 to account for partial offsets to the left

  // // if (AboutDetailsLength + copyLength < sizeof(Menu::AboutDetails)) {
  // strncat(AboutDetails, AboutDetails, AboutDetailsCopyLength);

  // startCharWidth = RRE.charWidth(AboutDetails[aboutCharPos]);
}

// Name menu item
void MenuFunctions::InitName()
{
  // Display.fillRect(0, 0, SCREEN_WIDTH, RREHeight_fixed_8x16, C_BLACK);
  // SetFontFixed();
  // RRE.printStr(ALIGN_CENTER, 0, DeviceName);
  // SetFontCustom();
  Menus::DisplayMenuBasicCenteredText(DeviceName);
}

// About menu item
void MenuFunctions::InitAbout()
{
  // Display.fillRect(0, 0, SCREEN_WIDTH, RREHeight_fixed_8x16, C_BLACK);
  // SetFontFixed();
  // RRE.printStr(ALIGN_CENTER, 0, AboutDetails);
  // SetFontCustom();
  snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "%s - Core %s - FW v%s - HW v%s - SW v%s - ",
           ModelNumber, getBuildVersion(), FirmwareRevision, HardwareRevision, SoftwareRevision);

  Menus::InitMenuItemDisplay("About", Menus::MenuTextBuffer, MENUS_SCROLLDEFINITELYNEEDED);
}

// Battery menu item
void MenuFunctions::DrawBatteryLevel()
{
  // SetFontFixed();
  // Display.fillRect(HALF_SCREEN_WIDTH, 0, HALF_SCREEN_WIDTH, RREHeight_fixed_8x16, C_BLACK);
  snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "%d %d%% %.1fv",
           Battery::CurrentBatterySensorReading,
           Battery::CurrentBatteryPercentage,
           Battery::Voltage);

  // RRE.printStr(ALIGN_RIGHT, 0, buffer);
  // SetFontCustom();
  Menus::UpdateMenuText(Menus::MenuTextBuffer, MENUS_NOSCROLLNEEDED);
}

void MenuFunctions::InitBattery()
{
  // Display.fillRect(0, 0, SCREEN_WIDTH, RREHeight_fixed_8x16, C_BLACK);
  // SetFontFixed();
  // static char label[] = "Battery:";
  // RRE.printStr(0, 0, label);

  Menus::InitMenuItemDisplay("Battery", NONE, MENUS_NOSCROLLNEEDED);
  MenuFunctions::DrawBatteryLevel();
}

void MenuFunctions::UpdateBattery()
{
  if (SecondRollover)
    MenuFunctions::DrawBatteryLevel();
}

// WiFi menu item
// void MenuFunctions::DrawWiFi()
// {
//   SetFontFixed();
//   Display.fillRect(MenuContentStartX, 0, SCREEN_WIDTH - MenuContentStartX, RREHeight_fixed_8x16, C_BLACK);
//   RRE.printStr(MenuContentStartX, 0, Network::WiFiStatus);
//   SetFontCustom();
// }

char *LastWiFiStatus = NONE;

void MenuFunctions::InitWiFi()
{
  Menus::InitMenuItemDisplay("WiFi", Network::WiFiStatus, MENUS_SCROLLCHECK);
  LastWiFiStatus = Network::WiFiStatus;
}

void MenuFunctions::UpdateWiFi()
{
  if (SecondRollover && LastWiFiStatus != Network::WiFiStatus)
  {
    Menus::UpdateMenuText(Network::WiFiStatus, MENUS_SCROLLCHECK);
    LastWiFiStatus = Network::WiFiStatus;
  }
}

int LastWebServerMode = -999;

// WebServer menu item
void MenuFunctions::DrawWebServer()
{
  WiFiMode_t mode = WiFi.getMode();
Serial.println("WEB: " + String(mode) + " cast " + String((int)mode));

  if ((int)mode == LastWebServerMode)
    return;     // No point in restarting if its already doing this as a thing

  LastWebServerMode = (int)mode;

  IPAddress local = WiFi.localIP();
  IPAddress accessPoint = WiFi.softAPIP();

  if (mode == WIFI_STA && local != 0)
    snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "IP %s", local.toString());
  else if (mode == WIFI_AP && accessPoint != 0)
    snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "Access Point ", local.toString());
  else if (mode == WIFI_AP_STA)
    snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "IP %s - Access Point %s - ", local.toString(), local.toString());
  else
    snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "IP Address not found");

  Menus::UpdateMenuText(Menus::MenuTextBuffer, MENUS_SCROLLCHECK);
}

void MenuFunctions::InitWebServer()
{
  Menus::InitMenuItemDisplay("Web", NONE);
  LastWebServerMode = -999;
  DrawWebServer();
}

void MenuFunctions::UpdateWebServer()
{
  if (SecondRollover)
    DrawWebServer();
}

// FPS menu item
void MenuFunctions::DrawFPS()
{
    itoa(FPS, Menus::MenuTextBuffer, 10);
    Menus::UpdateMenuText(Menus::MenuTextBuffer, MENUS_NOSCROLLNEEDED);
}

void MenuFunctions::InitFPS()
{
  Menus::InitMenuItemDisplay("FPS", NONE);
  DrawFPS();
}

void MenuFunctions::UpdateFPS()
{
  if (SecondRollover)
    DrawFPS();
}