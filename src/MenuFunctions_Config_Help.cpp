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
    DrawScrollArrows();

  // Serial.println("After DrawConfigHelp Pos: " + String(ConfigHelpTextPos) + " PixelOffset: " + String(ConfigHelpPixelOffset) + " offset: " + String(offset) + " tempDirection: " + String(tempDirection) + " tempTimer: " + String(tempTimer) + " textLineHeight: " + String(TextLineHeight));
  // Serial.println();

  // if (--tempTimer == 0)
  // {
  //   tempTimer = 500;
  //   tempDirection = -tempDirection;
  // }
}