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
#include "Profiles.h"
#include "Idle.h"

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

  if (PRESSED == Menus::SelectState())
  {
    //  Check for buttons being held down
    if (PRESSED == Menus::UpState())
      ConfigHelpPixelOffset += 2;
    if (PRESSED == Menus::DownState())
      ConfigHelpPixelOffset -= 2;

    SetFontSmall(); // Gives is small line height

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

    SetFontFixed();

    showScrollIcons = true;
  }
  else
    Menus::Config_CheckForMenuChange();

  MenuFunctions::Config_Draw_Help(showScrollIcons);
}

void MenuFunctions::Config_Draw_Help(int showScrollIcons)
{
  // Special case - the rendering of a scrollable window can overlap with menu name/icon/scrolling line area
  // (RREFont won't allow for clipping regions)
  // so we make sure this area is included when blanking things - and note relevant content will need to be redrawn after

  SetFontSmall(); // IMPORTANT - BELOW DEPENDS ON THIS AND SUBSEQUENT USAGE OF GLOBAL TextLineHeight
  // Clear main area
  Display.fillRect(0, MenuContentStartY, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY), C_BLACK);

  // Clear out anything scrolling off the top, and ready for menu area to be (re)drawn
  Display.fillRect(0, MenuContentStartY - TextLineHeight, SCREEN_WIDTH, TextLineHeight, C_BLACK);

  int offset = 20 - ConfigHelpPixelOffset;

  ResetPrintDisplayLine(offset, 0, SetFontSmall);
  int pos = ConfigHelpTextPos;
  for (int i = 0; i < 6; i++)
  {
    const TextLine *line = &ConfigHelpText[(ConfigHelpTextPos + i) % ConfigHelpTextSize];
    PrintDisplayLine(line);
  }

  // Clear out anything scrolling off the top, and ready for menu area to be (re)drawn
  Display.fillRect(0, MenuContentStartY - TextLineHeight, SCREEN_WIDTH, TextLineHeight, C_BLACK);

  if (showScrollIcons)
    DrawScrollArrows();
}