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
#define SCROLL_HOLD_TIME 300  // milliseconds before auto-scroll begins
#define SCROLL_REPEAT_TIME 50 // milliseconds between auto-scroll repeats

static char passwordBuffer[PASSWORD_MAX_LENGTH + 1] = {0}; // Password buffer, null-terminated
static int cursorPos = 0;                                  // Current cursor position (0-63)
static int currentCharIndex = 0;                           // Current character being scrolled through (0-94 for ASCII 32-126)
static bool selectHeld = false;                            // Whether select is currently held
static unsigned long buttonPressTime = 0;                  // When up/down button was first pressed (while select held)
static unsigned long lastScrollTime = 0;                   // Last time auto-scroll occurred
static bool autoScrolling = false;                         // Whether button is auto-scrolling
static int scrollDirection = 0;                            // -1 for up, 1 for down, 0 for none
static char previousCharAtCursor = 0;                      // The previous character at cursor position

// WiFi test state variables
static String lastTestedPassword = ""; // Track last tested password to know when to test again
static String lastTestedSSID = "";     // Track last tested SSID to know when to test again
static Network::WiFiTestResult wifiTestResult = Network::TEST_NOT_STARTED;
static unsigned long wifiTestResultDisplayTime = 0;                  // When the test result was last displayed
static const unsigned long WIFI_TEST_RESULT_DISPLAY_DURATION = 5000; // Show result for 5 seconds

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
  wifiTestResultDisplayTime = 0;

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

// Serial.println("SelectState: " + String(selectState) + " SelectJustChanged: " + String(selectJustChanged) +
//                " BackState: " + String(backState) + " BackJustChanged: " + String(backJustChanged) +
//                " UpState: " + String(upState) + " UpJustChanged: " + String(upJustChanged) +
//                " DownState: " + String(downState) + " DownJustChanged: " + String(downJustChanged));

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
        currentCharIndex = charToIndex('a'); // Start at 'a' if character is empty
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
  String currentPassword = String((const char *)passwordBuffer);
  Current_Profile->WiFi_Password = currentPassword;

  // Auto-test WiFi connection when password or SSID changes
  String currentSSID = Current_Profile->WiFi_Name;
  bool ssidChanged = (currentSSID != lastTestedSSID);
  bool passwordChanged = (currentPassword != lastTestedPassword);
  bool testInProgress = Network::IsWiFiTestInProgress();

  // If either SSID or password changed while test in progress, cancel and restart
  if ((ssidChanged || passwordChanged) && testInProgress)
  {
    Network::CancelWiFiTest();
    testInProgress = false;
  }

  // Start a new WiFi test if SSID and password are available and either has changed
  if (currentSSID.length() > 0 && (ssidChanged || passwordChanged))
  {
    // Start a new WiFi test
    if (!testInProgress)
    {
      wifiTestResult = Network::TestWiFiConnection(currentSSID, currentPassword);
      lastTestedPassword = currentPassword;
      lastTestedSSID = currentSSID;
      wifiTestResultDisplayTime = millis();

      Serial_INFO;
      Serial.printf("WiFi credentials changed - Testing connection with SSID: '%s'\n", currentSSID.c_str());
    }
  }
  else if (testInProgress)
  {
    // Continue polling the test result
    wifiTestResult = Network::TestWiFiConnection(currentSSID, currentPassword);
  }

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

  if ((millis() & 512) == 0) // Blink every half second
    Display.drawFastHLine(passwordUpToCursorWidth, SCREEN_HEIGHT - 2, 8, C_WHITE);

  // Draw scrolling character selection in the middle of the screen
  if (selectHeld)
  {
    SetFontFixed();

    // Middle of screen Y position
    int middleY = MenuContentStartY + 8;

    // Draw current character in the middle with scale 2
    RRE.setScale(2);
    char currentChar = indexToChar(currentCharIndex);
    int currentCharWidth = RRE.charWidth(currentChar) * 2;
    int centerX = SCREEN_WIDTH / 2;
    int currentCharX = centerX - (currentCharWidth / 2);
    RRE.drawChar(currentCharX, middleY - 8, currentChar);
    RRE.setScale(1);

    middleY += 4;

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

    // Display WiFi test results

    char *resultText = nullptr;

    unsigned long timeSinceTestResult = millis() - wifiTestResultDisplayTime;
    if (wifiTestResult != Network::TEST_NOT_STARTED && timeSinceTestResult < WIFI_TEST_RESULT_DISPLAY_DURATION)
    {
      // Display the test result in a visible area (below the instructions)
      SetFontTiny();
      SetFontLineHeightTiny();
      int resultY = MenuContentStartY + 50;

      // Clear the result area
      Display.fillRect(0, resultY - 2, SCREEN_WIDTH, 50, C_BLACK);

      switch (wifiTestResult)
      {
      case Network::TEST_SUCCESS:
        resultText = "Status: SUCCESS!";
        break;
      case Network::TEST_CONNECTING:
        resultText = "Status: Testing...";
        break;
      case Network::TEST_INVALID_PASSWORD:
        resultText = "Status: Invalid Password";
        break;
      case Network::TEST_SSID_NOT_FOUND:
        resultText = "Status: SSID Not Found";
        break;
      case Network::TEST_TIMEOUT:
        resultText = "Status: Connection Timeout";
        break;
      case Network::TEST_FAILED:
        resultText = "Status: Connection Failed";
        break;
      default:
        resultText = "Status: Unknown";
      }
    }
    else
      resultText = "Status: Pending";

    PrintDisplayLineCentered(resultText);
  }

  SetFontFixed();
  SetFontLineHeightFixed();
}

void MenuFunctions::Config_Exit_WiFi_Password()
{
  // Cancel any in-progress WiFi test when exiting the menu
  if (Network::IsWiFiTestInProgress())
  {
    Network::CancelWiFiTest();
  }

  // Just in case
  Network::Config_InitiWifi();
}