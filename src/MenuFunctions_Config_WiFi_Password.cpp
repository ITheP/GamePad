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

// int MenuFunctions::Profile_Selected = 0;
// Profile *MenuFunctions::Current_Profile = nullptr;

// char Profile_Text[] = "Save and restart\nto apply changes";

// WiFi Password entry state variables
#define PASSWORD_MAX_LENGTH 63
#define ASCII_MIN 32
#define ASCII_MAX 126
#define SCROLL_HOLD_TIME 500   // milliseconds before auto-scroll begins
#define SCROLL_REPEAT_TIME 100 // milliseconds between auto-scroll repeats

static char passwordBuffer[PASSWORD_MAX_LENGTH + 1] = {0}; // Password buffer, null-terminated
static int cursorPos = 0;                                  // Current cursor position (0-63)
static int currentCharIndex = 0;                           // Current character being scrolled through (0-94 for ASCII 32-126)
static bool selectHeld = false;                            // Whether select is currently held
static unsigned long upPressTime = 0;                      // When up button was first pressed (while select held)
static unsigned long downPressTime = 0;                    // When down button was first pressed (while select held)
static unsigned long lastUpScrollTime = 0;                 // Last time up auto-scroll occurred
static unsigned long lastDownScrollTime = 0;               // Last time down auto-scroll occurred
static bool upAutoScrolling = false;                       // Whether up button is auto-scrolling
static bool downAutoScrolling = false;                     // Whether down button is auto-scrolling
static char previousCharAtCursor = 0;                      // The previous character at cursor position

// Helper function to convert ASCII character to index (0-94)
static int charToIndex(char c)
{
  if (c == 0x00)
    return 0; // Default to 'a'
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
    strncpy(passwordBuffer, Current_Profile->WiFi_Password.c_str(), PASSWORD_MAX_LENGTH);

  cursorPos = 0;
  currentCharIndex = 0;
  selectHeld = false;
  upPressTime = 0;
  downPressTime = 0;
  lastUpScrollTime = 0;
  lastDownScrollTime = 0;
  upAutoScrolling = false;
  downAutoScrolling = false;
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
  int upState = Menus::UpState();
  bool upJustChanged = Menus::UpJustChanged();
  int downState = Menus::DownState();
  bool downJustChanged = Menus::DownJustChanged();

  // Select button handling - character scrolling
  if (selectState == PRESSED)
  {
    if (!selectHeld)
    {
      // Select just pressed, assume we are starting a new letter
      selectHeld = true;
      upPressTime = 0;
      downPressTime = 0;
      lastUpScrollTime = 0;
      lastDownScrollTime = 0;
      upAutoScrolling = false;
      downAutoScrolling = false;

      // Get the character at current cursor position
      char charAtCursor = passwordBuffer[cursorPos];
      if (charAtCursor == 0x00)
        currentCharIndex = charToIndex('a'); // Start at 'a' if character is empty
      else
        currentCharIndex = charToIndex(charAtCursor);
    }
    else
    {
      // Select is still held - handle up/down button scrolling

      // Up button handling
      if (upJustChanged && upState == PRESSED)
      {
        // Up button just pressed - immediately scroll up
        currentCharIndex--;
        if (currentCharIndex < 0)
          currentCharIndex = (ASCII_MAX - ASCII_MIN);
        upPressTime = now;
        lastUpScrollTime = now;
        upAutoScrolling = false;
      }
      else if (upState == PRESSED && upPressTime != 0)
      {
        // Up button is held
        if (!upAutoScrolling && (now - upPressTime) >= SCROLL_HOLD_TIME)
        {
          // Start auto-scrolling after hold time
          upAutoScrolling = true;
          lastUpScrollTime = now;
        }

        if (upAutoScrolling && (now - lastUpScrollTime) >= SCROLL_REPEAT_TIME)
        {
          // Auto-scroll at repeat interval
          currentCharIndex--;
          if (currentCharIndex < 0)
            currentCharIndex = (ASCII_MAX - ASCII_MIN);
          lastUpScrollTime = now;
        }
      }
      else if (upState == NOT_PRESSED)
      {
        // Up button released
        upPressTime = 0;
        upAutoScrolling = false;
      }

      // Down button handling
      if (downJustChanged && downState == PRESSED)
      {
        // Down button just pressed - immediately scroll down
        currentCharIndex++;
        if (currentCharIndex > (ASCII_MAX - ASCII_MIN))
          currentCharIndex = 0;
        downPressTime = now;
        lastDownScrollTime = now;
        downAutoScrolling = false;
      }
      else if (downState == PRESSED && downPressTime != 0)
      {
        // Down button is held
        if (!downAutoScrolling && (now - downPressTime) >= SCROLL_HOLD_TIME)
        {
          // Start auto-scrolling after hold time
          downAutoScrolling = true;
          lastDownScrollTime = now;
        }

        if (downAutoScrolling && (now - lastDownScrollTime) >= SCROLL_REPEAT_TIME)
        {
          // Auto-scroll at repeat interval
          currentCharIndex++;
          if (currentCharIndex > (ASCII_MAX - ASCII_MIN))
            currentCharIndex = 0;
          lastDownScrollTime = now;
        }
      }
      else if (downState == NOT_PRESSED)
      {
        // Down button released
        downPressTime = 0;
        downAutoScrolling = false;
      }
    }

    showScrollIcons = true;
  }
  else if (selectState == NOT_PRESSED && selectJustChanged)
  {
    // Select just released
    selectHeld = false;
    upPressTime = 0;
    downPressTime = 0;
    upAutoScrolling = false;
    downAutoScrolling = false;

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
          currentCharIndex = charToIndex('a');
        else
          currentCharIndex = charToIndex(charAtNewPos);
      }
    }
  }

  // Back button handling - cursor backward and ignore character
  if (Menus::BackState() == PRESSED && Menus::BackJustChanged())
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
  Current_Profile->WiFi_Password = String((const char *)passwordBuffer);

  if (DisplayRollover)
    MenuFunctions::Config_Draw_WiFi_Password(showScrollIcons);
}

void MenuFunctions::Config_Draw_WiFi_Password(int showScrollIcons)
{
  Display.fillRect(0, MenuContentStartY - 2, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY + 2), C_BLACK);

  // Draw password buffer at the bottom of the screen
  // Show only last 3 characters, previous ones replaced with *
  int passwordDisplayY = SCREEN_HEIGHT - 10;
  int passwordDisplayX = 2;

  // Show whole password - was going to hide certain characters BTW how to check it if can't see it!
  SetFontFixed();

  RRE.printStr(0, SCREEN_HEIGHT - TextLineHeight, passwordBuffer);
    
  // Get length of pixel width of password up to cursor position
    int passwordUpToCursorWidth = RRE.strWidth(passwordBuffer);
    // Draw cursor under current character

    if ((Second >> 2) %2 == 0) // Blink every half second
      Display.drawFastHLine(passwordUpToCursorWidth, SCREEN_HEIGHT - 2, 8, C_WHITE);

  // Draw scrolling character selection in the middle of the screen
  if (selectHeld)
  {
    SetFontFixed();

    // Middle of screen Y position
    int middleY = MenuContentStartY + 10;

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


  }
  else
  {
    SetFontTiny();
    SetFontLineHeightTiny();
    ResetPrintDisplayLine(MenuContentStartY);

    PrintDisplayLine("Green + up/down for next");
    PrintDisplayLine("Red to delete character");
  }
  
  SetFontFixed();
  SetFontLineHeightFixed();
}