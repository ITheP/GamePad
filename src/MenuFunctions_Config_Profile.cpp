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
#include <Profiles.h>

// Save State
enum CopyPasteState
{
  CP_NONE,
  CP_WAITING,
  CP_COPYING,
  CP_COPIED,
  CP_CANCELLED,
  CP_PASTING,
  CP_PASTED
};

int MenuFunctions::SelectedProfileId = 0;
//Profile *MenuFunctions::Current_Profile = nullptr;
// Profile *MenuFunctions::Default_Profile = nullptr;
Profile *CopyProfile = nullptr;

static CopyPasteState currentCopyPasteState = CP_WAITING;

char Profile_Text[] = "Save and restart\nto apply changes";

// Profile selector Menu Functions
void MenuFunctions::Config_Init_Profile()
{
  Menus::InitMenuItemDisplay(true);
  currentCopyPasteState = CP_NONE;
  Config_Draw_Profile(false);

  // Default_Profile = Profiles::AllProfiles[0];
}

void MenuFunctions::Config_Update_Profile()
{
  int showScrollIcons = false;

  // if (SecondRollover)
  if (PRESSED == Menus::SelectState())
  {
    if (PRESSED == Menus::UpState() && Menus::UpJustChanged())
      SelectedProfileId++;
    else if (PRESSED == Menus::DownState() && Menus::DownJustChanged())
      SelectedProfileId--;

    if (SelectedProfileId < 0)
      SelectedProfileId = 5;
    else if (SelectedProfileId > 5)
      SelectedProfileId = 0;

    Profiles::SetCurrentProfileFromId(SelectedProfileId);

#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.printf("Profile selected: %d - %s, WiFi: %s, WiFi Password: %s\n", CurrentProfile->Id, CurrentProfile->Description.c_str(), CurrentProfile->WiFi_Name.c_str(), SaferPasswordString(CurrentProfile->WiFi_Password).c_str());
#endif

    currentCopyPasteState = CP_NONE;

    showScrollIcons = true;
  }
  else if (PRESSED == Menus::BackState())
  {
    if (PRESSED == Menus::UpState())
    {
      // Copy operations

      if (Menus::UpJustChanged())
      {
        // Kick off save
        AnimationFrameIndex = 0;
        Icon = FilledCircleIcons[AnimationFrameIndex];
        FrameTimer = millis();

        currentCopyPasteState = CP_COPYING;
        MessageTop = "Copying " + CurrentProfile->Description + "...";
        MessageBottom = "Release to cancel";
      }
      else if (currentCopyPasteState == CP_COPYING)
      {
        // Copy operation - if held long enough populate a profile as the CopyProfile

        unsigned long elapsed = millis() - FrameTimer;
        if (elapsed >= DelayBetweenFrames)
        {
          // After a short delay, increase animation frame towards a final saving goal
          FrameTimer = millis();
          AnimationFrameIndex += 2;
          if (AnimationFrameIndex > 13) // Got past last frame of animation - we save
          {
            // Copy profile
            CopyProfile = CurrentProfile;
            currentCopyPasteState = CP_COPIED;
            MessageTop = CopyProfile->Description + " copied";
            MessageBottom = "Ready to paste";
          }
          else
          {
            Icon = FilledCircleIcons[AnimationFrameIndex];
            MessageBottom = "Release to cancel";
          }
        }
      }
    }
    else if (PRESSED == Menus::DownState())
    {
      // Paste initiation

      if (Menus::DownJustChanged())
      {
        if (CopyProfile == nullptr)
        {
          // Source not specified
          currentCopyPasteState = CP_CANCELLED;
          FrameTimer = millis();
          MessageTop = "Copy source";
          MessageBottom = "before pasting";
        }
        else if (CopyProfile == CurrentProfile)
        {
          // Attempting to copying on to itself
          currentCopyPasteState = CP_CANCELLED;
          FrameTimer = millis();
          MessageTop = "Can't copy profile";
          MessageBottom = "On to itself";
        }
        else
        {
          // All good, initiate pasting
          AnimationFrameIndex = 0;
          Icon = FilledCircleIcons[AnimationFrameIndex];
          FrameTimer = millis();
          currentCopyPasteState = CP_PASTING;
          MessageTop = "Pasting from " + CopyProfile->Description;
          MessageBottom = "Release to cancel";
        }
      }
      else if (currentCopyPasteState == CP_PASTING)
      {
        // Pasting processing

        unsigned long elapsed = millis() - FrameTimer;
        if (elapsed >= DelayBetweenFrames)
        {
          // After a short delay, increase animation frame towards a final saving goal
          FrameTimer = millis();
          AnimationFrameIndex += 2;
          if (AnimationFrameIndex > 13) // Got past last frame of animation - we save
          {
            CurrentProfile->CopySettingsFrom(CopyProfile);
            currentCopyPasteState = CP_PASTED;
            MessageTop = "Copied from " + CopyProfile->Description;
            MessageBottom = "into " + CurrentProfile->Description;
          }
          else
          {
            Icon = FilledCircleIcons[AnimationFrameIndex];
            MessageTop = "Pasting from " + CopyProfile->Description;
            MessageBottom = "Release to cancel";
          }
        }
      }
    }
    else
    {
      // Nothing pressed, make sure all timers are static
      if (currentCopyPasteState == CP_COPYING || currentCopyPasteState == CP_PASTING)
      {
        FrameTimer = millis();
        if (currentCopyPasteState == CP_COPYING)
          MessageTop = "Copy of " + CurrentProfile->Description;
        else
          MessageTop = "Paste of " + CopyProfile->Description;
        MessageBottom = "Cancelled";
        currentCopyPasteState = CP_CANCELLED;
      }
      else if (currentCopyPasteState != CP_CANCELLED)
      {
        currentCopyPasteState = CP_WAITING;
        MessageTop = "Copy: " DIGITALINPUT_CONFIG_UP_LABEL;
        MessageBottom = "Paste: " DIGITALINPUT_CONFIG_DOWN_LABEL;
      }
    }
  }
  else
  {
    Menus::Config_CheckForMenuChange();

    currentCopyPasteState = CP_NONE;
  }

  if (DisplayRollover)
    MenuFunctions::Config_Draw_Profile(showScrollIcons);
}

// Draw profile called less than update profile, so Update_ above may update several times before _Draw is called
void MenuFunctions::Config_Draw_Profile(int showScrollIcons)
{
  Display.fillRect(0, MenuContentStartY - 2, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY + 2), C_BLACK);

  if (currentCopyPasteState == CP_NONE)
  {
    SetFontFixed();

    RREDefault.setScale(2);

    sprintf(buffer, "%s", CurrentProfile->Description);
    RREDefault.printStr(ALIGN_CENTER, MenuContentStartY - 4, buffer);

    RREDefault.setScale(1);

    ResetPrintDisplayLine(SCREEN_HEIGHT - 18, 0, SetFontSmall);

    if (CurrentProfile->WiFi_Name.length() == 0)
      PrintDisplayLineCentered("No WiFi selected");
    else
    {
      sprintf(buffer, "WiFi: %s", CurrentProfile->WiFi_Name.c_str());
      PrintDisplayLineCentered(buffer);

      auto it = Network::AccessPointList.find(CurrentProfile->WiFi_Name);
      if (it != Network::AccessPointList.end())
      {
        // Mapping exists
        AccessPoint *ap = it->second;
        sprintf(buffer, "OK (%s)", ap->WiFiStatus);
        PrintDisplayLineCentered(buffer);
      }
      else
        PrintDisplayLineCentered("Searching for WiFi...");
    }

    if (showScrollIcons)
    {
      // Base size of left/right arrow icons is 7x7 px
      static int middle = ((SCREEN_HEIGHT - MenuContentStartY - 7) / 2) + MenuContentStartY;
      int iconOffset = (Menus::MenuFrame >> 2) % 3;
      RenderIcon(Icon_Arrow_Left + iconOffset, 0, middle, 7, 7);
      RenderIcon(Icon_Arrow_Right + iconOffset, SCREEN_WIDTH - 7, middle, 7, 7);
    }
  }
  else
  {
    // Save circle
    int checkX = (SCREEN_WIDTH / 2) - 7;
    int checkY = MenuContentStartY + 13;
    RenderIcon(Icon_Check_Spin1 + ((Menus::MenuFrame % 8) >> 1), checkX, checkY, 14, 14);

    if (currentCopyPasteState == CP_COPYING || currentCopyPasteState == CP_PASTING)
    {
      // Get width of the growing icon and center it dynamically
      int iconOffset = RREIcon.charWidth(Icon) / 2;
      int centeredX = (SCREEN_WIDTH / 2) - iconOffset;
      int centeredY = MenuContentStartY + 20 - iconOffset;
      RenderIcon(Icon, centeredX, centeredY, 0, 0);
    }
    else if (currentCopyPasteState == CP_COPIED || currentCopyPasteState == CP_PASTED)
      RenderIcon(Icon_Check_Yes, checkX, checkY, 0, 0);
    else if (currentCopyPasteState == CP_CANCELLED)
    {
      RenderIcon(Icon_Check_No, checkX, checkY, 0, 0);

      unsigned long elapsed = millis() - FrameTimer;
      if (elapsed > 2000)
        currentCopyPasteState = CP_WAITING;
    }
    else
    {
      // Show a filled copy area to represent copy buffer set
      if (CopyProfile != nullptr)
        RenderIcon(Icon_FilledCircle_9, checkX + 2, checkY + 2, 0, 0);
    }

    DrawMessages();
  }
}

void MenuFunctions::DrawMessages()
{
  RREDefault.printStr(ALIGN_CENTER, MenuContentStartY - 3, (char *)MessageTop.c_str());
  RREDefault.printStr(ALIGN_CENTER, SCREEN_HEIGHT - TextLineHeight, (char *)MessageBottom.c_str());
}