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

static SaveState currentSaveState = SAVE_WAITING;
char Icon;
int AnimationFrameIndex = 0; // Index for tracking current frame (0-13)
int FrameTimer = 0;
int DelayBetweenFrames = 125; // milliseconds between icon frames - total time to save = this * 7 animation frames

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
      AnimationFrameIndex = 0;
      Icon = FilledCircleIcons[AnimationFrameIndex];
      FrameTimer = millis();
      currentSaveState = SAVE_SAVING;
    }
    else if (currentSaveState == SAVE_SAVING)
    {
      unsigned long elapsed = millis() - FrameTimer;
      if (elapsed >= DelayBetweenFrames)
      {
        // After a short delay, increase animation frame towards a final saving goal
        FrameTimer = millis();
        AnimationFrameIndex += 2;
        if (AnimationFrameIndex > 13) // Got past last frame of animation - we save
        {
          // Initiate save
          Profiles::SaveAll();
          currentSaveState = SAVE_COMPLETED;
        }
        else
          Icon = FilledCircleIcons[AnimationFrameIndex];
      }
    }
  }
  else
  {
    if (currentSaveState == SAVE_SAVING)
    {
      currentSaveState = SAVE_CANCELLED;
      FrameTimer = millis();
    }

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
    int iconOffset = RRE.charWidth(Icon) / 2;
    int centeredX = (SCREEN_WIDTH / 2) - iconOffset;
    int centeredY = MenuContentStartY + 20 - iconOffset;
    RenderIcon(Icon, centeredX, centeredY, 0, 0);
    SetFontFixed();
    RRE.printStr(ALIGN_CENTER, MenuContentStartY - 3, "Saving...");
    RRE.printStr(ALIGN_CENTER, SCREEN_HEIGHT - TextLineHeight, "release to cancel");
  }
  else if (currentSaveState == SAVE_COMPLETED)
  {
    RenderIcon(Icon_Check_Yes, checkX, checkY, 0, 0);
    SetFontFixed();
    RRE.printStr(ALIGN_CENTER, MenuContentStartY - 3, "All done!");
    RRE.printStr(ALIGN_CENTER, SCREEN_HEIGHT - TextLineHeight, "Save Complete");
  }
  else if (currentSaveState == SAVE_CANCELLED)
  {
    RenderIcon(Icon_Check_No, checkX, checkY, 0, 0);
    SetFontFixed();
    RRE.printStr(ALIGN_CENTER, MenuContentStartY - 3, "Hold " DIGITALINPUT_CONFIG_SELECT_LABEL);
    RRE.printStr(ALIGN_CENTER, SCREEN_HEIGHT - TextLineHeight, "Save Cancelled");

    unsigned long elapsed = millis() - FrameTimer;
    if (elapsed > 2000)
      currentSaveState = SAVE_WAITING;
  }
  else
  {
    SetFontFixed();
    RRE.printStr(ALIGN_CENTER, MenuContentStartY - 3, "Hold " DIGITALINPUT_CONFIG_SELECT_LABEL);
    RRE.printStr(ALIGN_CENTER, SCREEN_HEIGHT - TextLineHeight, "to save");
  }

  SetFontFixed();
}