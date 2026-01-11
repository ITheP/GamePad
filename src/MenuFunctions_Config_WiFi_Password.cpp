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
#include <Arduino.h>

// TODO: Split off core rendering and handling to generalise functionality into reusable text string display/editor so can be re-used elsewhere

// WiFi Password entry state variables
#define PASSWORD_MAX_LENGTH 63
#define ASCII_MIN 32          // Minimum WiFi password friendly ascii character
#define ASCII_MAX 126         // Max WiFi password friendly ascii character
#define SCROLL_HOLD_TIME 250  // milliseconds before auto-scroll begins
#define SCROLL_REPEAT_TIME 30 // milliseconds between auto-scroll repeats

static char passwordBuffer[PASSWORD_MAX_LENGTH + 1] = {0};     // Raw password buffer, may include extra characters - cursorPos represents end point
static char displayBuffer[PASSWORD_MAX_LENGTH + 1] = {0};      // Buffer for display purposes
static char tempPasswordBuffer[PASSWORD_MAX_LENGTH + 1] = {0}; // Temporary buffer for snapshotting just the password
static int cursorPos = 0;                                      // Current cursor position (0-63)
static int currentCharIndex = 0;                               // Current character being scrolled through (0-94 for ASCII 32-126)
static bool selectHeld = false;                                // Whether select is currently held
static unsigned long buttonPressTime = 0;                      // When up/down button was first pressed (while select held)
static unsigned long lastScrollTime = 0;                       // Last time auto-scroll occurred
static bool autoScrolling = false;                             // Whether button is auto-scrolling
static int scrollDirection = 0;                                // -1 for up, 1 for down, 0 for none
static char previousCharAtCursor = 0;                          // The previous character at cursor position

// WiFi test state variables
static String lastTestedPassword = ""; // Track last tested password to know when to test again
static String lastTestedSSID = "";     // Track last tested SSID to know when to test again
static Network::WiFiTestResult wifiTestResult = Network::TEST_NOT_STARTED;

inline int charToIndex(char c)
{
  if (c == 0x00)
    return 0;

  return (int)c - ASCII_MIN;
}

inline char indexToChar(int idx)
{
  return (char)(ASCII_MIN + idx);
}

void MenuFunctions::Config_Init_WiFi_Password()
{
  Menus::InitMenuItemDisplay(true);

  // Initialize password buffer from current profile
  memset(passwordBuffer, 0x00, sizeof(passwordBuffer));

  if (CurrentProfile->WiFi_Password.length() > 0)
    strncpy(passwordBuffer, CurrentProfile->WiFi_Password.c_str(), PASSWORD_MAX_LENGTH);

  // For dev testing, pre-fill with a known password
  // strncpy(passwordBuffer, "TestPassword\0", PASSWORD_MAX_LENGTH);

  // Position cursor at end of current password
  cursorPos = strlen(passwordBuffer);
  if (cursorPos > PASSWORD_MAX_LENGTH)
    cursorPos = PASSWORD_MAX_LENGTH;

  // Set character index to the character at cursor position (or 'a' if empty)
  char charAtCursor = passwordBuffer[cursorPos];
  if (charAtCursor == 0x00)
    currentCharIndex = charToIndex('a');
  else
    currentCharIndex = charToIndex(charAtCursor);

  selectHeld = false;
  buttonPressTime = 0;
  lastScrollTime = 0;
  autoScrolling = false;
  scrollDirection = 0;
  previousCharAtCursor = passwordBuffer[cursorPos];

  // Reset WiFi test state
  lastTestedPassword = "";
  lastTestedSSID = "";
  wifiTestResult = Network::TEST_NOT_STARTED;

  Config_Draw_WiFi_Password(false);
}

void MenuFunctions::Config_Update_WiFi_Password()
{
  bool showScrollIcons = false;
  unsigned long now = millis();

  // Check if select button state has changed
  int selectState = Menus::SelectState();
  bool selectJustChanged = Menus::SelectJustChanged();
  int backState = Menus::BackState();
  bool backJustChanged = Menus::BackJustChanged();
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
      buttonPressTime = 0;
      lastScrollTime = 0;
      autoScrolling = false;
      scrollDirection = 0;

      // Get the character at current cursor position
      char charAtCursor = passwordBuffer[cursorPos];
      if (charAtCursor == 0x00)
        currentCharIndex = charToIndex('a'); // Default to 'a' if no character yet defined
      else
        currentCharIndex = charToIndex(charAtCursor);
    }
    else
    {
      // Select is still held - handle up/down button scrolling

      if (upState == PRESSED || downState == PRESSED)
      {
        int direction = (upState == PRESSED) ? -1 : 1;

        if (upJustChanged || downJustChanged)
        {
          // A button just pressed - immediately scroll
          currentCharIndex += direction;

          buttonPressTime = now;
          lastScrollTime = now;
          autoScrolling = false;
          scrollDirection = direction;
        }
        else if (buttonPressTime != 0)
        {
          if (!autoScrolling && (now - buttonPressTime) >= SCROLL_HOLD_TIME)
          {
            // Start auto-scrolling after hold time
            autoScrolling = true;
            lastScrollTime = now;
          }

          if (autoScrolling && (now - lastScrollTime) >= SCROLL_REPEAT_TIME)
          {
            lastScrollTime = now;
            currentCharIndex += direction;
          }
        }

        if (currentCharIndex < 0)
          currentCharIndex = (ASCII_MAX - ASCII_MIN);
        else if (currentCharIndex > (ASCII_MAX - ASCII_MIN))
          currentCharIndex = 0;
      }
      else if (upState == NOT_PRESSED && downState == NOT_PRESSED)
      {
        // Both buttons released
        buttonPressTime = 0;
        autoScrolling = false;
        scrollDirection = 0;
      }
    }

    showScrollIcons = true;
  }
  else if (selectState == NOT_PRESSED && selectJustChanged)
  {
    // Select just released
    selectHeld = false;
    buttonPressTime = 0;
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
          currentCharIndex = charToIndex('a');
        else
          currentCharIndex = charToIndex(charAtNewPos);
      }
    }
  }

  // Back button handling - cursor backward and ignore character
  if (backState == PRESSED && backJustChanged)
  {
    if (cursorPos > 0)
    {
      cursorPos--;

      // Get character at new cursor position for next iteration
      char charAtNewPos = passwordBuffer[cursorPos];
      if (charAtNewPos == 0x00)
        currentCharIndex = charToIndex('a');
      else
        currentCharIndex = charToIndex(charAtNewPos);
    }
  }

  // Check for menu navigation (only when select is not held)
  if (selectState != PRESSED)
  {
    Menus::Config_CheckForMenuChange();
  }

  // Save password to current profile (only up to cursor position)
  strncpy(tempPasswordBuffer, passwordBuffer, cursorPos);
  tempPasswordBuffer[cursorPos] = 0x00; // Explicitly null-terminate at cursor position
  String currentPassword = String((const char *)tempPasswordBuffer);
  CurrentProfile->WiFi_Password = currentPassword;

  // Auto-test WiFi connection when password or SSID changes
  String currentSSID = CurrentProfile->WiFi_Name;
  bool ssidChanged = (currentSSID != lastTestedSSID);
  bool passwordChanged = (currentPassword != lastTestedPassword);
  bool testInProgress = Network::IsWiFiTestInProgress();

  // If either SSID or password changed while test in progress, cancel and restart
  if ((ssidChanged || passwordChanged) && testInProgress)
    Network::CancelWiFiTest();

  // Start a new WiFi test if SSID and password are available and either has changed
  if (currentSSID.length() > 0 && (ssidChanged || passwordChanged))
  {
    // Start a new WiFi test
    Network::TestWiFiConnection(currentSSID, currentPassword);
    lastTestedPassword = currentPassword;
    lastTestedSSID = currentSSID;

#ifdef EXTRA_SERIAL_DEBUG
    Serial_INFO;
    Serial.printf("WiFi credentials changed - Testing connection with SSID: '%s'\n", currentSSID.c_str());
#endif
  }

  if (DisplayRollover)
    MenuFunctions::Config_Draw_WiFi_Password(showScrollIcons);
}

void MenuFunctions::Config_Draw_WiFi_Password(int showScrollIcons)
{
  Display.fillRect(0, MenuContentStartY - 2, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY + 2), C_BLACK);

  // Currently, show the whole password
  // Was going to hide certain characters
  // e.g. show only last 3 characters, previous ones replaced with *
  // BUT how to check password if you can't see it all, and can only move cursor by deleting last character...

  // Copy password buffer up to cursor position for display
  strncpy(displayBuffer, passwordBuffer, cursorPos);
  displayBuffer[cursorPos] = 0x00; // Explicitly null-terminate at cursor position

  // If password wider than screen, shrink font
  // TODO: improve this if password too long (show right hand side of it)
  if (RREDefault.strWidth(displayBuffer) + 10 > SCREEN_WIDTH)
    SetFontSmall();
  else
    SetFontFixed();

  RRE.printStr(0, SCREEN_HEIGHT - TextLineHeight, displayBuffer);

  // Get length of pixel width of password up to cursor position
  int passwordUpToCursorWidth = RRE.strWidth(displayBuffer);

  // Draw cursor under current character
  if ((millis() & 512) == 0) // Blink every half second
    Display.drawFastHLine(passwordUpToCursorWidth, SCREEN_HEIGHT - 2, 8, C_WHITE);

  // Draw scrolling character selection in the middle of the screen
  if (selectHeld)
  {
    // SetFontFixed();

    // Middle of screen Y position
    int middleY = MenuContentStartY;

    // Draw current character in the middle with scale 2
    RREDefault.setScale(2);
    char currentChar = indexToChar(currentCharIndex);
    int currentCharWidth = RREDefault.charWidth(currentChar) * 2;
    int centerX = SCREEN_WIDTH / 2;
    int currentCharX = centerX - (currentCharWidth / 2);
    RREDefault.drawChar(currentCharX, middleY + 1, currentChar);
    RREDefault.setScale(1);

    middleY += 13;

    // Draw characters to the right (ASCII_MIN onwards)
    int drawX = centerX + (currentCharWidth / 2) + 4;
    int charIdx = (currentCharIndex + 1) % (ASCII_MAX - ASCII_MIN + 1);
    while (drawX < SCREEN_WIDTH) // - 10)
    {
      char c = indexToChar(charIdx);
      RREDefault.drawChar(drawX, middleY, c);
      drawX += RREDefault.charWidth(c) + 2;
      charIdx = (charIdx + 1) % (ASCII_MAX - ASCII_MIN + 1);
    }

    // Draw characters to the left
    drawX = currentCharX - 2; // 2 for extra letter spacing
    charIdx = (currentCharIndex - 1);
    if (charIdx < 0)
      charIdx = (ASCII_MAX - ASCII_MIN);

    while (drawX > 0)
    {
      char c = indexToChar(charIdx);
      int cWidth = RREDefault.charWidth(c);
      drawX -= (cWidth + 2); // 2 for extra letter spacing
      RREDefault.drawChar(drawX, middleY, c);
      charIdx--;
      if (charIdx < 0)
        charIdx = (ASCII_MAX - ASCII_MIN);
    }

    // Draw left and right arrows if select is held
    static int middle = ((SCREEN_HEIGHT - MenuContentStartY - 7) / 2) + MenuContentStartY;

    int iconOffset = (Menus::MenuFrame >> 2) % 3;

    RREIcon.setColor(C_BLACK);
    RenderIcon(Icon_Arrow_Left_Outline + iconOffset, -3, middle - 3, 0, 0);
    RenderIcon(Icon_Arrow_Right_Outline + iconOffset, SCREEN_WIDTH - 7 - 2, middle - 3, 0, 0);

    RREIcon.setColor(C_WHITE);
    RenderIcon(Icon_Arrow_Left + iconOffset, 0, middle, 0, 0);
    RenderIcon(Icon_Arrow_Right + iconOffset, SCREEN_WIDTH - 7, middle, 0, 0);
  }
  else
  {
    ResetPrintDisplayLine(MenuContentStartY, 0, SetFontSmall);

    PrintDisplayLine("Green + up/down for next");
    PrintDisplayLine("Red to delete character");

    // Display WiFi test results
    // char *resultText = nullptr;
    String currentPassword = String((const char *)passwordBuffer);
    String currentSSID = CurrentProfile->WiFi_Name;

    //  Display the test result in a visible area (below the instructions)
    int resultY = MenuContentStartY + 50;

    // Clear the result area
    Display.fillRect(0, resultY - 2, SCREEN_WIDTH, 50, C_BLACK);

    wifiTestResult = Network::CheckTestResults();

    String resultText = Network::DescribeTestResults(currentSSID, currentPassword, wifiTestResult);

    sprintf(buffer, "Status: %s", resultText);
    PrintDisplayLineCentered(buffer);
  }
}

void MenuFunctions::Config_Exit_WiFi_Password()
{
  // Cancel any in-progress WiFi test when exiting the menu
  if (Network::IsWiFiTestInProgress())
  {
    Network::CancelWiFiTest();
  }

  // Just in case
  Network::Config_InitWifi();
}