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
#include <Arduino.h>

int MenuFunctions::Profile_Selected = 0;
Profile* MenuFunctions::Current_Profile = nullptr;

char Profile_Text[] = "Save and restart\nto apply changes";

// WiFi Password entry state variables
#define PASSWORD_MAX_LENGTH 63
#define ASCII_MIN 32
#define ASCII_MAX 126
#define SCROLL_HOLD_TIME 500  // milliseconds before auto-scroll begins

static char passwordBuffer[64] = {0};  // Password buffer, null-terminated
static int cursorPos = 0;              // Current cursor position (0-63)
static int currentCharIndex = 0;       // Current character being scrolled through (0-94 for ASCII 32-126)
static bool selectHeld = false;        // Whether select is currently held
static unsigned long selectPressTime = 0;  // When select was first pressed
static bool autoScrolling = false;     // Whether we're currently auto-scrolling
static int scrollDirection = 0;        // 1 for forward, -1 for back, 0 for none
static char previousCharAtCursor = 0;  // The previous character at cursor position

// Helper function to convert ASCII character to index (0-94)
static int charToIndex(char c)
{
  if (c == 0x00)
    return 0;  // Default to 'a'
  return (int)c - ASCII_MIN;
}

// Helper function to convert index back to ASCII character
static char indexToChar(int idx)
{
  return (char)(ASCII_MIN + idx);
}

// Profile selector Menu Functions
void MenuFunctions::Config_Init_WiFi_Password()
{
  Menus::InitMenuItemDisplay(true);

  // Initialize password buffer from current profile
  memset(passwordBuffer, 0x00, sizeof(passwordBuffer));
  if (Current_Profile->WiFi_Password.length() > 0)
  {
    strncpy(passwordBuffer, Current_Profile->WiFi_Password.c_str(), PASSWORD_MAX_LENGTH);
  }

  cursorPos = 0;
  currentCharIndex = 0;
  selectHeld = false;
  autoScrolling = false;
  scrollDirection = 0;
  previousCharAtCursor = passwordBuffer[0];

  Config_Draw_WiFi_Password(false);
}

void MenuFunctions::Config_Update_WiFi_Password()
{
  bool showScrollIcons = false;
  unsigned long now = millis();

  // Check if select button state has changed
  int selectState = Menus::SelectState();
  bool selectJustChanged = Menus::SelectJustChanged();
  bool upJustChanged = Menus::UpJustChanged();
  bool downJustChanged = Menus::DownJustChanged();
  int upState = Menus::UpState();
  int downState = Menus::DownState();

  // Select button handling - character scrolling
  if (selectState == PRESSED)
  {
    if (!selectHeld)
    {
      // Select just pressed
      selectHeld = true;
      selectPressTime = now;
      autoScrolling = false;
      scrollDirection = 0;

      // Get the character at current cursor position
      char charAtCursor = passwordBuffer[cursorPos];
      if (charAtCursor == 0x00)
      {
        currentCharIndex = charToIndex('a');  // Start at 'a' if character is empty
      }
      else
      {
        currentCharIndex = charToIndex(charAtCursor);
      }
    }
    else
    {
      // Select is still held
      // Check if we should start auto-scrolling
      if (!autoScrolling && (now - selectPressTime) >= SCROLL_HOLD_TIME)
      {
        autoScrolling = true;
      }

      // Handle up/down button clicks while select is held (forward/back scrolling)
      if (upJustChanged && upState == PRESSED && downState == PRESSED)
      {
        // Both up and down pressed simultaneously - scroll backwards
        currentCharIndex--;
        if (currentCharIndex < 0)
          currentCharIndex = (ASCII_MAX - ASCII_MIN);
        scrollDirection = -1;
      }
      else if (downJustChanged && downState == PRESSED && upState == PRESSED)
      {
        // Both up and down pressed simultaneously - scroll forwards
        currentCharIndex++;
        if (currentCharIndex > (ASCII_MAX - ASCII_MIN))
          currentCharIndex = 0;
        scrollDirection = 1;
      }
      else if (autoScrolling)
      {
        // Auto-scroll based on which direction was held
        if (upState == PRESSED && downState != PRESSED)
        {
          currentCharIndex--;
          if (currentCharIndex < 0)
            currentCharIndex = (ASCII_MAX - ASCII_MIN);
          scrollDirection = -1;
        }
        else if (downState == PRESSED && upState != PRESSED)
        {
          currentCharIndex++;
          if (currentCharIndex > (ASCII_MAX - ASCII_MIN))
            currentCharIndex = 0;
          scrollDirection = 1;
        }
      }
    }

    showScrollIcons = true;
  }
  else if (selectState == NOT_PRESSED && selectJustChanged)
  {
    // Select just released
    selectHeld = false;
    autoScrolling = false;
    scrollDirection = 0;

    // Save the current character to the password buffer
    if (cursorPos <= PASSWORD_MAX_LENGTH)
    {
      passwordBuffer[cursorPos] = indexToChar(currentCharIndex);
      previousCharAtCursor = passwordBuffer[cursorPos];

      // Move cursor forward one position (unless at position 63)
      if (cursorPos < PASSWORD_MAX_LENGTH)
      {
        cursorPos++;
        // Get character at new cursor position for next iteration
        char charAtNewPos = passwordBuffer[cursorPos];
        if (charAtNewPos == 0x00)
        {
          currentCharIndex = charToIndex('a');
        }
        else
        {
          currentCharIndex = charToIndex(charAtNewPos);
        }
      }
    }
  }

  // Back button handling - cursor backward and ignore character
  if (selectState == NOT_PRESSED && Menus::BackState == PRESSED && Menus::BackJustChanged())
  {
    if (cursorPos > 0)
    {
      cursorPos--;
      // Character at cursor remains in buffer but is ignored until select is pressed again
      // When select is pressed again, it will show the previous character that was there
    }
  }

  // Check for menu navigation (only when select is not held)
  if (selectState != PRESSED)
  {
    Menus::Config_CheckForMenuChange();
  }

  // Save password to current profile
  Current_Profile->WiFi_Password = String((const char*)passwordBuffer);

  if (DisplayRollover)
    MenuFunctions::Config_Draw_WiFi_Password(showScrollIcons);
}

void MenuFunctions::Config_Draw_WiFi_Password(int showScrollIcons)
{
  Display.fillRect(0, MenuContentStartY - 2, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY + 2), C_BLACK);

  RRE.setScale(2);

  sprintf(buffer, "%s", Current_Profile->Description);
  RRE.printStr(ALIGN_CENTER, MenuContentStartY - 4, buffer);

  RRE.setScale(1);

  SetFontTiny();
  SetFontLineHeightTiny();

  ResetPrintDisplayLine(SCREEN_HEIGHT - (TextLineHeight * 2) + 2);

  if (Current_Profile->WiFi_Name.length() == 0)
  {
    PrintDisplayLineCentered("No WiFi selected");
  }
  else
  {
    sprintf(buffer, "WiFi: %s", Current_Profile->WiFi_Name.c_str());
    PrintDisplayLineCentered(buffer);

    auto it = Network::AccessPointList.find(Current_Profile->WiFi_Name);
    if (it != Network::AccessPointList.end())
    {
      AccessPoint *ap = it->second;
      PrintDisplayLineCentered(ap->WiFiStatus);
    }
    else
      PrintDisplayLineCentered("WiFi Not Found");
  }

  SetFontLineHeightFixed();
  SetFontFixed();

  // Draw password buffer at the bottom of the screen
  // Show only last 3 characters, previous ones replaced with *
  int passwordDisplayY = SCREEN_HEIGHT - 10;
  int passwordDisplayX = 2;

  int displayStart = (cursorPos > 3) ? (cursorPos - 3) : 0;
  int displayEnd = cursorPos + 1;
  if (displayEnd > PASSWORD_MAX_LENGTH)
    displayEnd = PASSWORD_MAX_LENGTH;

  int drawX = passwordDisplayX;
  for (int i = displayStart; i < displayEnd; i++)
  {
    char charToDraw;
    if (i < cursorPos)
    {
      charToDraw = '*';  // Show asterisk for previous characters
    }
    else
    {
      charToDraw = passwordBuffer[i];
      if (charToDraw == 0x00)
        charToDraw = '_';  // Show underscore for empty positions
    }

    RRE.drawChar(drawX, passwordDisplayY, charToDraw);
    drawX += RRE.charWidth(charToDraw) + 1;
  }

  // Draw underline cursor at the current position
  int cursorDrawX = passwordDisplayX;
  for (int i = displayStart; i < cursorPos; i++)
  {
    char c = passwordBuffer[i];
    if (c == 0x00)
      c = '_';
    cursorDrawX += RRE.charWidth(c) + 1;
  }
  // Draw underline under current character position
  Display.drawLine(cursorDrawX, passwordDisplayY + 11, cursorDrawX + 5, passwordDisplayY + 11, C_WHITE);

  // Draw scrolling character selection in the middle of the screen
  if (selectHeld)
  {
    SetFontFixed();
    
    // Middle of screen Y position
    int middleY = (SCREEN_HEIGHT - MenuContentStartY) / 2 + MenuContentStartY;
    
    // Draw current character in the middle with scale 2
    RRE.setScale(2);
    char currentChar = indexToChar(currentCharIndex);
    int currentCharWidth = RRE.charWidth(currentChar) * 2;
    int centerX = SCREEN_WIDTH / 2;
    int currentCharX = centerX - (currentCharWidth / 2);
    RRE.drawChar(currentCharX, middleY - 8, currentChar);
    RRE.setScale(1);

    // Draw characters to the right (ASCII_MIN onwards)
    int drawX = centerX + (currentCharWidth / 2) + 2;
    int charIdx = (currentCharIndex + 1) % (ASCII_MAX - ASCII_MIN + 1);
    while (drawX < SCREEN_WIDTH - 10)
    {
      char c = indexToChar(charIdx);
      RRE.drawChar(drawX, middleY, c);
      drawX += RRE.charWidth(c) + 1;
      charIdx = (charIdx + 1) % (ASCII_MAX - ASCII_MIN + 1);
    }

    // Draw characters to the left
    drawX = currentCharX - 2;
    charIdx = (currentCharIndex - 1);
    if (charIdx < 0)
      charIdx = (ASCII_MAX - ASCII_MIN);
    
    while (drawX > 10)
    {
      char c = indexToChar(charIdx);
      int cWidth = RRE.charWidth(c);
      drawX -= (cWidth + 1);
      if (drawX >= 10)
      {
        RRE.drawChar(drawX, middleY, c);
      }
      charIdx--;
      if (charIdx < 0)
        charIdx = (ASCII_MAX - ASCII_MIN);
    }

    // Draw left and right arrows if select is held
    SetFontCustom();
    static int middle = ((SCREEN_HEIGHT - MenuContentStartY - 7) / 2) + MenuContentStartY;
    RenderIcon(Icon_Arrow_Left, 0, middle, 7, 7);
    RenderIcon(Icon_Arrow_Right, SCREEN_WIDTH - 7, middle, 7, 7);
    SetFontFixed();
  }
}