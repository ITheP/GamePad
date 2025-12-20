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

int MenuFunctions::Profile_Selected = 0;
Profile* MenuFunctions::Current_Profile = nullptr;

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

// char Text_Profile_Default[] = "Default";

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

  ResetPrintDisplayLine(SCREEN_HEIGHT - (TextLineHeight * 2) + 2);

  if (Current_Profile->WiFi_Name.length() == 0)
  {
    PrintDisplayLineCentered("No WiFi selected");

    // PrintDisplayLineCentered("AbCdgjp,.");
  }
  else
  {
    sprintf(buffer, "WiFi: %s", Current_Profile->WiFi_Name.c_str());
    PrintDisplayLineCentered(buffer);

    auto it = Network::AccessPointList.find(Current_Profile->WiFi_Name);
    if (it != Network::AccessPointList.end())
    {
      // Mapping exists
      AccessPoint *ap = it->second;
      PrintDisplayLineCentered(ap->WiFiStatus);
    }
    else
      PrintDisplayLineCentered("WiFi Not Found");
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