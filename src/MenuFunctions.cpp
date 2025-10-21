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

static char Text_Connected[] = "Connected";
static char Text_Disconnected[] = "Disconnected";

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
  Menus::DisplayMenuBasicCenteredText(DeviceName);
}

// About menu item
void MenuFunctions::InitAbout()
{
  snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "%s - Core %s - FW v%s - HW v%s - SW v%s",
           ModelNumber, getBuildVersion(), FirmwareRevision, HardwareRevision, SoftwareRevision);

  Menus::InitMenuItemDisplay(Menus::MenuTextBuffer, ScrollDefinitelyNeeded);
}

// Version menu item
void MenuFunctions::InitVersion()
{
  snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "About text TODO",
           ModelNumber, getBuildVersion(), FirmwareRevision, HardwareRevision, SoftwareRevision);

  Menus::InitMenuItemDisplay(Menus::MenuTextBuffer, ScrollDefinitelyNeeded);
}

static char Battery_Empty[] = "Empty";
static char Battery_Low[] = "Low";
static char Battery_OK[] = "OK";
static char Battery_Good[] = "Good";
static char Battery_Awesome[] = "Awesome";
static char Battery_Charging[] = "Charging";
static char Battery_Wired[] = "Wired";

// Battery menu item
void MenuFunctions::DrawBatteryLevel()
{
  // snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "%d %d%% %.1fv",
  //          Battery::CurrentBatterySensorReading,
  //          Battery::CurrentBatteryPercentage,
  //          Battery::Voltage);

  float batteryLevel = Battery::Voltage;
  const char *text;

  if (batteryLevel <= 3.3)
    text = Battery_Empty;
  else if (batteryLevel < 3.5)
    text = Battery_Low;
  else if (batteryLevel < 3.7)
    text = Battery_OK;
  else if (batteryLevel < 3.9)
    text = Battery_Good;
  else
    text = Battery_Awesome;

  snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "%s %d%% %.1fv",
           text,
           Battery::CurrentBatteryPercentage,
           Battery::Voltage);

  Menus::UpdateMenuText(Menus::MenuTextBuffer, NoScrollNeeded);
}

void MenuFunctions::InitBattery()
{
  Menus::InitMenuItemDisplay();
  MenuFunctions::DrawBatteryLevel();
}

void MenuFunctions::UpdateBattery()
{
  if (SecondRollover)
    MenuFunctions::DrawBatteryLevel();
}

// Wifi menu item
char *LastWiFiStatus = NONE;

void MenuFunctions::InitWiFi()
{
  Menus::InitMenuItemDisplay(Network::WiFiStatus, ScrollCheck);
  LastWiFiStatus = Network::WiFiStatus;
}

void MenuFunctions::UpdateWiFi()
{
  if (SecondRollover && LastWiFiStatus != Network::WiFiStatus)
  {
    Menus::UpdateMenuText(Network::WiFiStatus, ScrollCheck);
    LastWiFiStatus = Network::WiFiStatus;
  }
}

// Bluetooth menu item
bool LastBluetoothStatus = false;

void MenuFunctions::DrawBluetooth()
{
  LastBluetoothStatus = BTConnectionState;
  snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "%s - Name: %s", (LastBluetoothStatus ? Text_Connected : Text_Disconnected), DeviceName); //, VID, PID);
  Menus::UpdateMenuText(Menus::MenuTextBuffer, ScrollDefinitelyNeeded);
}

void MenuFunctions::InitBluetooth()
{
  Menus::InitMenuItemDisplay();
  DrawBluetooth();
}

void MenuFunctions::UpdateBluetooth()
{
  if (SecondRollover && LastBluetoothStatus != BTConnectionState)
    DrawBluetooth();
}

int LastWebServerMode = -999;

// WebServer menu item
void MenuFunctions::DrawWebServer()
{
  WiFiMode_t mode = WiFi.getMode();
  // Serial.println("WEB: " + String(mode) + " cast " + String((int)mode));

  if ((int)mode == LastWebServerMode)
    return; // No point in restarting if its already doing this as a thing

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

  Menus::UpdateMenuText(Menus::MenuTextBuffer, ScrollCheck);
}

void MenuFunctions::InitWebServer()
{
  Menus::InitMenuItemDisplay();
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
  Menus::UpdateMenuText(Menus::MenuTextBuffer, NoScrollNeeded);
}

void MenuFunctions::InitFPS()
{
  Menus::InitMenuItemDisplay();
  DrawFPS();
}

void MenuFunctions::UpdateFPS()
{
  if (SecondRollover)
    DrawFPS();
}

int LastUSBStatus = -1;

// USB menu item
void MenuFunctions::DrawUSB()
{
  int usbStatus = Serial;

  if (LastUSBStatus == usbStatus)
    return;

  LastUSBStatus = usbStatus;

  if (usbStatus)
    Menus::UpdateMenuText(Text_Connected, NoScrollNeeded);
  else
    Menus::UpdateMenuText(Text_Disconnected, NoScrollNeeded);
}

void MenuFunctions::InitUSB()
{
  Menus::InitMenuItemDisplay();
  LastUSBStatus = -1;
  DrawUSB();
}

void MenuFunctions::UpdateUSB()
{
  if (SecondRollover)
    DrawUSB();
}

static char test[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890!\"£$%^&*()-=_+<>,.;'#:@~[]{}";

void MenuFunctions::InitDebug()
{
  Menus::InitMenuItemDisplay(test, ScrollDefinitelyNeeded);
}

static char stats[] = "STATISTICS - TODO";

void MenuFunctions::InitStats()
{
  Menus::InitMenuItemDisplay(stats, ScrollDefinitelyNeeded);
}