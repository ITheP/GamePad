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
  // Debug::CrashDeviceOnPurpose();
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