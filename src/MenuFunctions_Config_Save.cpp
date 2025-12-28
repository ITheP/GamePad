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

// Save State
enum SaveState
{
  SAVE_WAITING,
  SAVE_SAVING,
  SAVE_CANCELLED,
  SAVE_COMPLETED
};

SaveState currentSaveState = SAVE_WAITING;
char SavingIcon;
int SavingFrameIndex = 0;  // Index for tracking current frame (0-13)
int FrameDelay = 0;
int DelayBetweenFrames = 75; // milliseconds between icon frames

// Array of filled circle icons in order
static const char FilledCircleIcons[] = {
  Icon_FilledCircle_1,
  Icon_FilledCircle_2,
  Icon_FilledCircle_3,
  Icon_FilledCircle_4,
  Icon_FilledCircle_5,
  Icon_FilledCircle_6,
  Icon_FilledCircle_7,
  Icon_FilledCircle_8,
  Icon_FilledCircle_9,
  Icon_FilledCircle_10,
  Icon_FilledCircle_11,
  Icon_FilledCircle_12,
  Icon_FilledCircle_13,
  Icon_FilledCircle_14
};

// Save Settings
void MenuFunctions::Config_Init_SaveSettings()
{
  Menus::InitMenuItemDisplay(true);
  currentSaveState = SAVE_WAITING;
  Config_Draw_SaveSettings();
}

void MenuFunctions::Config_Update_SaveSettings()
{
  int showScrollIcons = false;

  // if (SecondRollover)
  if (PRESSED == Menus::SelectState() && currentSaveState != SAVE_COMPLETED)
  {
    if (Menus::SelectJustChanged())
    {
      // Kick off save
      SavingFrameIndex = 0;
      SavingIcon = FilledCircleIcons[SavingFrameIndex];
      FrameDelay = millis();
      currentSaveState = SAVE_SAVING;
    }
    else if (currentSaveState == SAVE_SAVING)
    {
      unsigned long elapsed = millis() - FrameDelay;
      if (elapsed >= DelayBetweenFrames)
      {
        // After a short delay, increase animation frame towards a final saving goal
        FrameDelay = millis();
        SavingFrameIndex += 2;
        if (SavingFrameIndex > 13) // Got past last frame of animation - we save
        {
          // Initiate save
          Profiles::SaveAll();
          currentSaveState = SAVE_COMPLETED;
        }
        else
          SavingIcon = FilledCircleIcons[SavingFrameIndex];
      }
    }
  }
  else
  {
    if (currentSaveState == SAVE_SAVING)
      currentSaveState = SAVE_CANCELLED;

    Menus::Config_CheckForMenuChange(); // Handle changing menu option
  }

  if (DisplayRollover)
    MenuFunctions::Config_Draw_SaveSettings();
}

// char Text_Profile_Default[] = "Default";

void MenuFunctions::Config_Draw_SaveSettings()
{
  Display.fillRect(0, MenuContentStartY - 2, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY + 2), C_BLACK);

  SetFontCustom();

  // Save circle
  int checkX = (SCREEN_WIDTH / 2) - 7;
  int checkY = MenuContentStartY + 13;
  RenderIcon(Icon_Check_Spin1 + ((Menus::MenuFrame % 8) >> 1), checkX, checkY, 14, 14);

  if (currentSaveState == SAVE_SAVING)
  {
    // Get width of the growing icon and center it dynamically
    int iconOffset = RRE.charWidth(SavingIcon) / 2;
    int centeredX = (SCREEN_WIDTH / 2) - iconOffset;
    int centeredY = MenuContentStartY + 20 - iconOffset;
    RenderIcon(SavingIcon, centeredX, centeredY, 0, 0);
    SetFontFixed();
    RRE.printStr(ALIGN_CENTER, MenuContentStartY - 3, "Saving...");
  }
  else if (currentSaveState == SAVE_COMPLETED)
  {
    RenderIcon(Icon_Check_Yes, checkX, checkY, 0,0);
    SetFontFixed();
    RRE.printStr(ALIGN_CENTER, MenuContentStartY - 3, "All done!");
    RRE.printStr(ALIGN_CENTER, SCREEN_HEIGHT - TextLineHeight, "Save Complete");
  }
  else if (currentSaveState == SAVE_CANCELLED)
  {
    RenderIcon(Icon_Check_No, checkX, checkY, 0,0);
    SetFontFixed();
    RRE.printStr(ALIGN_CENTER, MenuContentStartY - 3, "Hold green to save");
    RRE.printStr(ALIGN_CENTER, SCREEN_HEIGHT - TextLineHeight, "Save Cancelled");
  }
  else
  {
    SetFontFixed();
    RRE.printStr(ALIGN_CENTER, MenuContentStartY - 3, "Hold green to save");
  }

  SetFontFixed();
}