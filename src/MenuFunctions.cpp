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
#include "GamePad.h"
#include "Battery.h"
#include "Network.h"
#include "Web.h"
#include "Debug.h"
#include <Profiles.h>

extern CRGB ExternalLeds[];

#define MenuContentStartX 22
#define MenuContentStartY 20

// NOTE
// Careful with your choice of Scroll checking choices. If your text is a little bit too long
// and you select NoScroll - remembering it renders from the right of the screen - there is a chance
// it will draw into the menu icon area.

int Profile_Selected = 0;
Profile *Current_Profile;

void MenuFunctions::Setup()
{
}

// Config Menu Functions

void MenuFunctions::Config_Setup()
{
    // Even if not required, we kick off WiFi scanning for config mode
    Network::Config_InitiWifi();
    Profiles::LoadAll();
    Current_Profile = Profiles::AllProfiles[Profile_Selected];
}

// Config.Help

int ConfigHelpTextSize = sizeof(ConfigHelpText) / sizeof(ConfigHelpText[0]);
int ConfigHelpTextPos = 0;
int ConfigHelpPixelOffset = 0;

void MenuFunctions::Config_Init_Help()
{
  Menus::InitMenuItemDisplay(true);
}

void MenuFunctions::Config_Update_Help()
{
  int showScrollIcons = false;

  // if (SecondRollover)
  if (PRESSED == Menus::SelectState())
  {
    Serial.println("SELECT PRESSED - ConfigHelpPixelOffset: " + String(ConfigHelpPixelOffset) + " ConfigHelpTextPos: " + String(ConfigHelpTextPos));
    //  Check for buttons being held down
    if (PRESSED == Menus::UpState())
      ConfigHelpPixelOffset += 2;
    if (PRESSED == Menus::DownState())
      ConfigHelpPixelOffset -= 2;

    SetFontLineHeightTiny();

    if (ConfigHelpPixelOffset >= TextLineHeight)
    {
      ConfigHelpPixelOffset -= TextLineHeight;
      ConfigHelpTextPos++;
    }
    else if (ConfigHelpPixelOffset < 0)
    {
      ConfigHelpPixelOffset += (TextLineHeight - 1);
      ConfigHelpTextPos--;
      if (ConfigHelpTextPos < 0)
        ConfigHelpTextPos = ConfigHelpTextSize - 1;
    }

    SetFontLineHeightFixed();

    showScrollIcons = true;
  }
  else
  {
    Menus::Config_CheckForMenuChange(); // Handle changing menu option
  }

  MenuFunctions::Config_Draw_Help(showScrollIcons);
}

// int tempDirection = 1;
// int tempTimer = 500;

void MenuFunctions::Config_Draw_Help(int showScrollIcons)
{
  SetFontTiny();
  SetFontLineHeightTiny(); // IMPORTANT - BELOW DEPENDS ON THIS SETTING AND SUBSEQUENT USAGE OF GLOBAL TextLineHeight

  // Special case - the rendering of a scrollable window can overlap with menu name/icon/scrolling line area
  // (RREFont won't allow for clipping regions)
  // so we make sure this area is included when blanking things - and note relevant content will need to be redrawn after

  // CLear main area
  Display.fillRect(0, MenuContentStartY, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY), C_BLACK);

  // Clear out anything scrolling off the top, and ready for menu area to be (re)drawn
  Display.fillRect(0, MenuContentStartY - TextLineHeight, SCREEN_WIDTH, TextLineHeight, C_BLACK);

  int offset = RREHeight_fixed_8x16 + 6 - ConfigHelpPixelOffset;
  // Serial.println("Before DrawConfigHelp Pos: " + String(ConfigHelpTextPos) + " PixelOffset: " + String(ConfigHelpPixelOffset) + " offset: " + String(offset) + " tempDirection: " + String(tempDirection) + " tempTimer: " + String(tempTimer) + " textLineHeight: " + String(TextLineHeight));
  ResetPrintDisplayLine(offset);
  int pos = ConfigHelpTextPos;
  for (int i = 0; i < 6; i++)
    PrintDisplayLine((char *)ConfigHelpText[(ConfigHelpTextPos + i) % ConfigHelpTextSize]);

  // Clear out anything scrolling off the top, and ready for menu area to be (re)drawn
  Display.fillRect(0, MenuContentStartY - TextLineHeight, SCREEN_WIDTH, TextLineHeight, C_BLACK);

  SetFontLineHeightFixed();

  if (showScrollIcons)
  {
    SetFontCustom();
    RRE.setColor(C_BLACK);
    RRE.drawChar(SCREEN_WIDTH - 9, MenuContentStartY - 2, Icon_Arrow_Up_Outline);
    RRE.drawChar(SCREEN_WIDTH - 9, SCREEN_HEIGHT - 8, Icon_Arrow_Down_Outline);

    RRE.setColor(C_WHITE);
    RRE.drawChar(SCREEN_WIDTH - 7, MenuContentStartY, Icon_Arrow_Up);
    RRE.drawChar(SCREEN_WIDTH - 7, SCREEN_HEIGHT - 7, Icon_Arrow_Down);
  }

  SetFontFixed();

  // Serial.println("After DrawConfigHelp Pos: " + String(ConfigHelpTextPos) + " PixelOffset: " + String(ConfigHelpPixelOffset) + " offset: " + String(offset) + " tempDirection: " + String(tempDirection) + " tempTimer: " + String(tempTimer) + " textLineHeight: " + String(TextLineHeight));
  // Serial.println();

  // if (--tempTimer == 0)
  // {
  //   tempTimer = 500;
  //   tempDirection = -tempDirection;
  // }
}

char Profile_Text[] = "Save and restart\nto apply changes";

// Profile selector Menu Functions
void MenuFunctions::Config_Init_Profile()
{
  Menus::InitMenuItemDisplay(true);
  Config_Draw_Profile(false);
}

void MenuFunctions::Config_Update_Profile()
{
  int showScrollIcons = false;

  // if (SecondRollover)
  if (PRESSED == Menus::SelectState())
  {
    if (PRESSED == Menus::UpState() && Menus::UpJustChanged())
      Profile_Selected++;
    else if (PRESSED == Menus::DownState() && Menus::DownJustChanged())
      Profile_Selected--;

    if (Profile_Selected < 0)
      Profile_Selected = 5;
    else if (Profile_Selected > 5)
      Profile_Selected = 0;

    Current_Profile = Profiles::AllProfiles[Profile_Selected];

    showScrollIcons = true;
  }
  else
  {
    Menus::Config_CheckForMenuChange(); // Handle changing menu option
  }

  if (DisplayRollover)
    MenuFunctions::Config_Draw_Profile(showScrollIcons);
}

//char Text_Profile_Default[] = "Default";

void MenuFunctions::Config_Draw_Profile(int showScrollIcons)
{
  Display.fillRect(0, MenuContentStartY - 2, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY + 2), C_BLACK);

  RRE.setScale(2);

//  if (Profile_Selected == 0)
  //  RRE.printStr(ALIGN_CENTER, MenuContentStartY, Text_Profile_Default);
//  else
 // {
 //   sprintf(buffer, "%d", Profile_Selected);
 //   RRE.printStr(ALIGN_CENTER, MenuContentStartY, buffer);
 // }

    sprintf(buffer, "%s", Current_Profile->Description);
    RRE.printStr(ALIGN_CENTER, MenuContentStartY - 4, buffer);

  RRE.setScale(1);

  SetFontTiny();
  SetFontLineHeightTiny();

  ResetPrintDisplayLine(SCREEN_HEIGHT - (TextLineHeight * 2) + 2 );

  if (Current_Profile->WiFi_Name.length() == 0) {
    PrintDisplayLineCenter("No WiFi set");
    
    PrintDisplayLineCenter("AbCdgjp,.");
  }
  else
  {
    sprintf(buffer, "WiFi: %s", Current_Profile->WiFi_Name.c_str());
    PrintDisplayLineCenter(buffer);

    auto it = Network::AccessPointList.find(Current_Profile->WiFi_Name);
    if (it != Network::AccessPointList.end())
    {
      // Mapping exists
      AccessPoint &ap = it->second;
      PrintDisplayLine(ap.WiFiStatus);
    }
    else
      PrintDisplayLineCenter("WiFi Not Found");
  }

  SetFontLineHeightFixed();

  if (showScrollIcons)
  {
    SetFontCustom();
    // Base size of left/right arrow icons is 7x7 px
    static int middle = ((SCREEN_HEIGHT - MenuContentStartY - 7) / 2) + MenuContentStartY;
    int iconOffset = (Menus::MenuFrame >> 2) % 3;
    RenderIcon(Icon_Arrow_Left + iconOffset, 0, middle, 7, 7);
    RenderIcon(Icon_Arrow_Right + iconOffset, SCREEN_WIDTH - 7, middle, 7, 7);
  }

  SetFontFixed();
}

// WiFi Saved Settings
void MenuFunctions::Config_Init_WiFi_Settings()
{
  Menus::InitMenuItemDisplay(true);
}

void MenuFunctions::Config_Update_WiFi_Settings()
{
  if (SecondRollover)
    MenuFunctions::Config_Draw_WiFi_Settings();

  Menus::Config_CheckForMenuChange();
}

void MenuFunctions::Config_Draw_WiFi_Settings()
{
  Display.fillRect(0, MenuContentStartY, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY), C_BLACK);

  RRE.setScale(2);

  RRE.printStr(ALIGN_CENTER, MenuContentStartY, "WiFi Settings");

  RRE.setScale(1);
}

// WiFi Access Point Settings
void MenuFunctions::Config_Init_WiFi_AccessPoint()
{
  Menus::InitMenuItemDisplay(true);

  Display.fillRect(0, MenuContentStartY - 2, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY + 2), C_BLACK); // clears a bit of extra space in case other menu displayed outside usual boundaries
}

void MenuFunctions::Config_Update_WiFi_AccessPoint()
{
  if (SecondRollover)
    MenuFunctions::Config_Draw_WiFi_AccessPoint();

  Menus::Config_CheckForMenuChange();
}

void MenuFunctions::Config_Draw_WiFi_AccessPoint()
{
  Display.fillRect(0, MenuContentStartY, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY), C_BLACK);

  RRE.setScale(2);

  RRE.printStr(ALIGN_CENTER, MenuContentStartY, "WiFi Access Point");

  RRE.setScale(1);
}

// WiFi Username
void MenuFunctions::Config_Init_WiFi_Username()
{
  Menus::InitMenuItemDisplay(true);
}

void MenuFunctions::Config_Update_WiFi_Username()
{
  if (SecondRollover)
    MenuFunctions::Config_Draw_WiFi_Username();

  Menus::Config_CheckForMenuChange();
}

void MenuFunctions::Config_Draw_WiFi_Username()
{
  Display.fillRect(0, MenuContentStartY, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY), C_BLACK);

  RRE.setScale(2);

  RRE.printStr(ALIGN_CENTER, MenuContentStartY, "WiFi Username");

  RRE.setScale(1);
}

// WiFi Password
void MenuFunctions::Config_Init_WiFi_Password()
{
  Menus::InitMenuItemDisplay(true);
}

void MenuFunctions::Config_Update_WiFi_Password()
{
  if (SecondRollover)
    MenuFunctions::Config_Draw_WiFi_Password();

  Menus::Config_CheckForMenuChange();
}

void MenuFunctions::Config_Draw_WiFi_Password()
{
  Display.fillRect(0, MenuContentStartY, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY), C_BLACK);

  RRE.setScale(2);

  RRE.printStr(ALIGN_CENTER, MenuContentStartY, "WiFi Password");

  RRE.setScale(1);
}

// Save Settings
void MenuFunctions::Config_Init_SaveSettings()
{
  Menus::InitMenuItemDisplay(true);
}

void MenuFunctions::Config_Update_SaveSettings()
{
  if (SecondRollover)
    MenuFunctions::Config_Draw_SaveSettings();

  Menus::Config_CheckForMenuChange();
}

void MenuFunctions::Config_Draw_SaveSettings()
{
  Display.fillRect(0, MenuContentStartY, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY), C_BLACK);

  RRE.setScale(2);

  RRE.printStr(ALIGN_CENTER, MenuContentStartY, "Save Settings");

  RRE.setScale(1);
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

  // For testing device crash handling - handy trigger point here
  // Debug::CrashOnPurpose();
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

static char test[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890!\"£$%^&*()-=_+<>,.;'#:@~[]{}";

void MenuFunctions::InitDebug()
{
  Menus::InitMenuItemDisplay(Debug::CrashInfo, ScrollDefinitelyNeeded);
}

static char stats[] = "STATISTICS - TODO";

void MenuFunctions::InitStats()
{
  Menus::InitMenuItemDisplay(stats, ScrollDefinitelyNeeded);
}