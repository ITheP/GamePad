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
#include "Profiles.h"

extern CRGB ExternalLeds[];

char MenuFunctions::Icon;
int MenuFunctions::AnimationFrameIndex = 0; // Index for tracking current frame (0-13)
int MenuFunctions::FrameTimer = 0;
int MenuFunctions::DelayBetweenFrames = 125; // milliseconds between icon frames - total time to save = this * 7 animation frames
String MenuFunctions::MessageTop;
String MenuFunctions::MessageBottom;

// NOTE
// Careful with your choice of Scroll checking choices. If your text is a little bit too long
// and you select NoScroll - remembering it renders from the right of the screen - there is a chance
// it will draw into the menu icon area.

void MenuFunctions::Setup()
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif
}

// Config Menu Functions

void MenuFunctions::Config_Setup()
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

  // Even if not required, we kick off WiFi scanning for config mode
  Network::Config_InitWifi();
  Profiles::LoadAll();
  Profiles::SetCurrentProfileFromId(MenuFunctions::SelectedProfileId);
}

void MenuFunctions::DrawScrollArrows()
{
  // SetFontCustom();
  RREIcon.setColor(C_BLACK);
  RREIcon.drawChar(SCREEN_WIDTH - 9, MenuContentStartY - 2, Icon_Arrow_Up_Outline);
  RREIcon.drawChar(SCREEN_WIDTH - 9, SCREEN_HEIGHT - 8, Icon_Arrow_Down_Outline);

  RREIcon.setColor(C_WHITE);
  RREIcon.drawChar(SCREEN_WIDTH - 7, MenuContentStartY, Icon_Arrow_Up);
  RREIcon.drawChar(SCREEN_WIDTH - 7, SCREEN_HEIGHT - 7, Icon_Arrow_Down);

  // SetFontFixed();
}

// Main Menu Functions

// Name menu item
void MenuFunctions::InitName()
{
  Menus::DisplayMenuBasicCenteredText(DeviceName);
}

static char Menu_About[] = ABOUT;

// About menu item
void MenuFunctions::InitAbout()
{
  Menus::InitMenuItemDisplay(Menu_About, ScrollDefinitelyNeeded);
}

// Version menu item
void MenuFunctions::InitVersion()
{
  snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "%s - Core %s - FW v%s - HW v%s - SW v%s",
           ModelNumber, getBuildVersion(), FirmwareRevision, HardwareRevision, SoftwareRevision);

  Menus::InitMenuItemDisplay(Menus::MenuTextBuffer, ScrollDefinitelyNeeded);
}

// Wifi menu item
char *LastWiFiStatus = NONE;

void MenuFunctions::InitWiFi()
{
  Menus::InitMenuItemDisplay(Network::WiFiStatus, ScrollCheck);
  LastWiFiStatus = Network::WiFiStatus;

  Debug::CrashOnPurpose();
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

static char BT_Connected[] = "BT Connected";
static char BT_Disconnected[] = "BT Disconnected";

void MenuFunctions::DrawBluetooth()
{
  LastBluetoothStatus = BTConnectionState;
  snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "%s - %s", (LastBluetoothStatus ? BT_Connected : BT_Disconnected), FullDeviceName); //, VID, PID);
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

// WebServer menu item
// Does slightly more than show WebServer state - will show IP address to connect to web server
int LastWebServerMode = -999;

void MenuFunctions::DrawWebServer()
{

  WiFiMode_t mode = WiFi.getMode();

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
  {
    if (Web::WebServerEnabled)
      snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "Web Server enabled but no IP Address found");
    else
      snprintf(Menus::MenuTextBuffer, MenuTextBufferSize, "Web Server disabled");
  }

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

// USB menu item
int LastUSBStatus = -1;

static char USB_Connected[] = "USB Connected";
static char USB_Disconnected[] = "USB Disconnected";

void MenuFunctions::DrawUSB()
{
  int usbStatus = Serial;

  if (LastUSBStatus == usbStatus)
    return;

  LastUSBStatus = usbStatus;

  if (usbStatus)
    Menus::UpdateMenuText(USB_Connected, NoScrollNeeded);
  else
    Menus::UpdateMenuText(USB_Disconnected, ScrollDefinitelyNeeded);
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

void MenuFunctions::InitDebug()
{
  static char debugInfo[] = "Crash logs available via Web Debug";
  Menus::InitMenuItemDisplay(debugInfo, ScrollDefinitelyNeeded);
}

static char stats[] = "Want funky statistics? Check the web site!";

void MenuFunctions::InitStats()
{
  Menus::InitMenuItemDisplay(stats, ScrollDefinitelyNeeded);
}