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
#include <GamePad.h>
#include <Battery.h>
#include <Network.h>

// NOTE what seems like needlessly complicated optimisations here faffing around with buffers and screen width checks etc.
// to display menu items is all to cut down overheads
// Without optimisations when menus were engaged, FPS was dropping from around 10k to 30-40 FPS.
// Also, scrolling text does not come cheap.
// Every little helps!

extern CRGB ExternalLeds[];

#define MenuTextStartX 18 // 15 pixels for icon, 2 spacer pixels, 4 for separator, 2 spacer pixels

unsigned int Menus::MenuFrame = 0;
float MenuStartTime = 0.0;
#define MenuStartDelay 0.75 // Seconds before menu starts scrolling (if it scrolls)

int Menus::MenusStatus = OFF;
Menu *Menus::RootMenu = NONE;
Menu *Menus::CurrentMenu = NONE;

// Having a default MenuOption that does nothing helps save us check for null reference on
// uninitialised CurrentMenuOption later on
MenuOption *NoMenuOption = new MenuOption{"None", NONE, NONE, NONE};

MenuOption *Menus::CurrentMenuOption;

MenuOption MainMenuOptions[] = {
    {"Name", NONE, NONE, MenuFunctions::InitName, NONE, NONE}, // Name is first menu item we initialise to to make sure Device Name is drawn. Has own special rendering of text so we don't get menu icons etc. being drawn
    {"Battery", Icon_Menu_Battery, "Battery", MenuFunctions::InitBattery, MenuFunctions::UpdateBattery, NONE},
    {"BT", Icon_Menu_Bluetooth, "Bluetooth", MenuFunctions::InitBluetooth, MenuFunctions::UpdateBluetooth, NONE},
    {"WiFi", Icon_Menu_WiFi, "WiFi", MenuFunctions::InitWiFi, MenuFunctions::UpdateWiFi, NONE},
    {"USB", Icon_Menu_USB, "USB", MenuFunctions::InitUSB, MenuFunctions::UpdateUSB, NONE},
    {"FPS", Icon_Menu_FPS, "FPS", MenuFunctions::InitFPS, MenuFunctions::UpdateFPS, NONE},
    {"Web Server", Icon_Menu_WebServer, "Web Server", MenuFunctions::InitWebServer, MenuFunctions::UpdateWebServer, NONE},
    {"About", Icon_Menu_QuestionMark, "About", MenuFunctions::InitAbout, NONE, NONE},
    {"Version", Icon_Menu_Version, "About", MenuFunctions::InitVersion, NONE, NONE},
    {"Debug", Icon_Menu_Debug, "Debug", MenuFunctions::InitDebug, NONE, NONE},
    {"Stats", Icon_Menu_Stats, "Stats", MenuFunctions::InitStats, NONE, NONE}};

Menu Menus::MainMenu = {
    MainMenuOptions,
    sizeof(MainMenuOptions) / sizeof(MainMenuOptions[0]),
    NoMenuOption,
    -1,
    0};

// MenuOption ConfigMenuOptions[] = {
//     {"Help", Icon_Menu_QuestionMark, "Help", MenuFunctions::Config_Init_Help, MenuFunctions::Config_Update_Help, NONE}, // Name is first menu item we initialise to to make sure Device Name is drawn. Has own special rendering of text so we don't get menu icons etc. being drawn
//     {"Profile", Icon_Menu_WiFi, "Profile", MenuFunctions::Config_Init_Profile, MenuFunctions::Config_Update_Profile, NONE},
//     {"WiFi Settings", Icon_Menu_WiFi, "WiFi Settings", MenuFunctions::Config_Init_WiFi_Settings, MenuFunctions::Config_Update_WiFi_Settings, NONE},
//     {"WiFi Access Point", Icon_Menu_WiFi, "WiFi Access Point", MenuFunctions::Config_Init_WiFi_AccessPoint, MenuFunctions::Config_Update_WiFi_AccessPoint, NONE},
//     {"WiFi Username", Icon_Menu_WiFi, "WiFi Username", MenuFunctions::Config_Init_WiFi_Username, MenuFunctions::Config_Update_WiFi_Username, NONE},
//     {"WiFi Password", Icon_Menu_WiFi, "WiFi Password", MenuFunctions::Config_Init_WiFi_Password, MenuFunctions::Config_Update_WiFi_Password, NONE},
//     {"Save Config", Icon_Menu_WiFi, "Save Settings", MenuFunctions::Config_Init_SaveSettings, MenuFunctions::Config_Update_SaveSettings, NONE}};

MenuOption ConfigMenuOptions[] = {
    {"Help", Icon_Menu_QuestionMark, "Help", MenuFunctions::Config_Init_Help, MenuFunctions::Config_Update_Help, NONE}, // Name is first menu item we initialise to to make sure Device Name is drawn. Has own special rendering of text so we don't get menu icons etc. being drawn
    {"Profile", Icon_Menu_WiFi, "Profile", MenuFunctions::Config_Init_Profile, MenuFunctions::Config_Update_Profile, NONE},
    {"WiFi Settings", Icon_Menu_WiFi, "WiFi Settings", MenuFunctions::Config_Init_WiFi_Settings, MenuFunctions::Config_Update_WiFi_Settings, NONE},
    {"WiFi Access Point", Icon_Menu_WiFi, "WiFi Access Point", MenuFunctions::Config_Init_WiFi_AccessPoint, MenuFunctions::Config_Update_WiFi_AccessPoint, NONE},
    {"WiFi Username", Icon_Menu_WiFi, "WiFi Username", MenuFunctions::Config_Init_WiFi_Username, MenuFunctions::Config_Update_WiFi_Username, NONE},
    {"WiFi Password", Icon_Menu_WiFi, "WiFi Password", MenuFunctions::Config_Init_WiFi_Password, MenuFunctions::Config_Update_WiFi_Password, NONE},
    {"Save Config", Icon_Menu_WiFi, "Save Settings", MenuFunctions::Config_Init_SaveSettings, MenuFunctions::Config_Update_SaveSettings, NONE}};


Menu Menus::ConfigMenu = {
    ConfigMenuOptions,
    sizeof(ConfigMenuOptions) / sizeof(ConfigMenuOptions[0]),
    NoMenuOption,
    -1,
    0};

// TODO
// Parent/Child references to menus
// Enter submenu / return to parent menu operations
// Value selection operations for menu items that have values to select from (make this a reusable other menus, e.g. true/false
// Breadcrumb trail display of menu hierarchy

// Menu mode within main app
ControllerReport Menus::ToggleMenuMode()
{
  if (MenusStatus == ON)
  {
    MenusStatus = OFF;

    // Clear menu dotted line animation
    Display.drawFastHLine(0, 14, SCREEN_WIDTH, C_BLACK);

    return ReportToController;
  }

  MenusStatus = ON;
  return DontReportToController;
}

#define MaxCharsFitOnScreen ((SCREEN_WIDTH / 2) + 1) // Theoretical max number of chars on screen (1 pixel wide) + 1 pixel spacings +1 for /0
                                                     // Number of chars that fit on screen + 1 to account for partial offsets to the left;
char Menus::MenuTextBuffer[MenuTextBufferSize + 4];  // General use Menu display text buffer to render dynamic content into - +4 to make sure there is space for ' - \0'

int ScrollMenuText = OFF;
int DisplayTextStart = MenuTextStartX;
int MenuTextLength = 0;
int MenuTextCharPos = 0;
int MenuTextStartCharWidth = 0;
int MenuTextPixelPos = MenuTextStartCharWidth;

void Menus::InitMenuItemDisplay(int useMenuOptionLabel)
{
  if (useMenuOptionLabel)
    Menus::InitMenuItemDisplay(CurrentMenuOption->Label, ScrollCheck);
  else
    Menus::InitMenuItemDisplay(NONE);
}

// Menu auto-scrolls text if too long to fit on screen
void Menus::InitMenuItemDisplay(char *text, MenuScrollState scrollStatus)
{
  // WORK OUT ICONS OR LEFT MENU TEXT STUFF HERE
  SetFontCustom();
  Display.fillRect(0, 0, MenuTextStartX - 1, 13, C_BLACK); // SCREEN_WIDTH, 12, C_BLACK);

  RRE.drawChar(0, 2, CurrentMenuOption->Icon);
  // 15 pixels for icon, 3 spacer pixels, 4 for separator, 3 spacer pixels
  // RRE.drawChar(18, 3, Icon_Menu_Separator);

  if (text != NONE)
    UpdateMenuText(text, scrollStatus);
}

// Call ... checkScrollNeeded = false, scrollDefinitelyNeeded = false if you KNOW text will fit on screen
// Call ... checkScrollNeeded = true,  scrollDefinitelyNeeded = false if you aren't too sure text will fit
// Call ... checkScrollNeeded = false, scrollDefinitelyNeeded = true  if you KNOW text will NOT fit on screen
// We do all this to try minimise amount of overhead checking for text fit to work out if we need to scroll or not - high overheads
void Menus::UpdateMenuText(char *text, int scrollStatus)
{
  SetFontFixed();

  if (scrollStatus == ScrollCheck)
  {
    // Work out width of text to see if scrolling is needed
    // Optimised so stop once off screen
    int totalWidth = DisplayTextStart;
    MenuTextLength = 0;

    // We piggyback calculating the length of MenuTextLength in our check

    char c = text[0];
    while (c != '\0')
    {
      MenuTextLength++;
      totalWidth += RRE.charWidth(c) + 1; // +1 for character spacing

      if (totalWidth > SCREEN_WIDTH)
      {
        scrollStatus = ScrollDefinitelyNeeded;

        // Count out the rest of the characters in the string
        c = text[MenuTextLength];
        while (c != '\0')
          c = text[++MenuTextLength];

        break;
      }

      c = text[MenuTextLength];
    }
  }
  else if (scrollStatus == ScrollDefinitelyNeeded)
  {
    MenuTextLength = std::strlen(text);
  }

  // Make sure MenuTextBuffer contains the text, for redraws, scrolling, etc.
  if (text != MenuTextBuffer)
    strcpy(MenuTextBuffer, text); // Shouldn't need strncopy()

  if (scrollStatus == ScrollDefinitelyNeeded)
  {
    // For scrolling text, stick a " - " on to visually separate end of text from start on wrap
    // Already MenuTextBuffer we can just amend it. For other strings, won't have buffer
    // space at the end so we copy into the MenuTextBuffer.
    // Note that text could get used elsewhere (logs, serial output, web output)
    // so we don't go round sticking the separator on everything in advance as we might not want it.

    strcat(MenuTextBuffer, " - ");

    MenuTextLength += 3; // include the " - " above

    // Scroll that sucker!
    // Pad the end of the text buffer with the starting segment so we can scroll seamlessly
    // strncat(MenuTextBuffer, MenuTextBuffer, BufferCopyCharsLength);
    MenuTextCharPos = 0;
    MenuTextStartCharWidth = -RRE.charWidth(MenuTextBuffer[0]);
    MenuTextPixelPos = 0;
    ScrollMenuText = ON;

    MenuStartTime = Now + MenuStartDelay; // start scrolling in 2.5 seconds

    // Draw initial text, which will stick around till the MenuStartTime kicks in with actual scrolling updates
    DrawScrollingText();
  }
  else
  {
    // No scrolling needed - display now
    ScrollMenuText = OFF;

    DrawStaticMenuText();
  }

  //SetFontCustom();
}

void Menus::DrawStaticMenuText()
{
  // Redraw static text
  Display.fillRect(DisplayTextStart, 0, SCREEN_WIDTH - DisplayTextStart, 13, C_BLACK);
  // Display.fillRect(DisplayTextStart, 0, 10, RREHeight_fixed_8x16, C_WHITE);
  RRE.printStr(ALIGN_RIGHT, -1, MenuTextBuffer);
}

// Optimised to draw less when in Main screen
void Menus::DisplayMenuTextOptimised()
{
  // Throttle scrolling to every other frame
  if (ScrollMenuText == ON && (MenuFrame & 0x01) == 0 && (MenuStartTime < Now))
  {
    DrawScrollingText();
  }
}

// Always draw here to account for possible overwriting of menu area
void Menus::DisplayMenuText()
{
  // Throttle scrolling to every other frame
  // but redraw menu text as is each time to counter possible overwriting of menu area
  if (ScrollMenuText == ON)
  {
    if ((MenuFrame & 0x01 == 0) || MenuStartTime < Now)
      ReDrawScrollingText();
    else
      DrawScrollingText();
  }
  else
  {
    // Redraw static text
    Display.fillRect(DisplayTextStart, 0, SCREEN_WIDTH - DisplayTextStart, 13, C_BLACK);
    RRE.printStr(ALIGN_RIGHT, -1, MenuTextBuffer);
  }
}

void Menus::DrawScrollingText()
{
  RenderScrollingText();
  UpdateScrollingText();
  FinishScrollingText();
}

// Redraws current text without scrolling (e.g. we want the scroll rate to be slower
// than the display refresh rate, but we need to re-render it)
void Menus::ReDrawScrollingText()
{
  RenderScrollingText();
  FinishScrollingText();
}

void Menus::RenderScrollingText()
{
  // Scroll the text
  Display.fillRect(DisplayTextStart, 0, SCREEN_WIDTH - DisplayTextStart, 13, C_BLACK);
  SetFontFixed();

  int currentX = DisplayTextStart + MenuTextPixelPos;
  int maxX = SCREEN_WIDTH;
  int charWidth;

  int i = MenuTextCharPos;
  while (currentX < maxX) // MenuTextBuffer[i] != '\0'; i++)
  {
    RRE.drawChar(currentX, -1, MenuTextBuffer[i]);
    currentX += RRE.charWidth(MenuTextBuffer[i]) + 1; // +1 for character spacing

    i++;
    if (i >= MenuTextLength)
      i = 0;
  }
}

void Menus::UpdateScrollingText()
{
  MenuTextPixelPos--;
  if (MenuTextPixelPos < MenuTextStartCharWidth)
  {
    MenuTextPixelPos = 0;
    MenuTextCharPos++;

    if (MenuTextCharPos >= MenuTextLength)
      MenuTextCharPos = 0;

    MenuTextStartCharWidth = -RRE.charWidth(MenuTextBuffer[MenuTextCharPos]);
  }
}

void Menus::FinishScrollingText()
{
  SetFontCustom();

  // After scrolling content, the left edge rendering will creep into where the menu separator is - so we re-draw this to keep everything neat
  Display.fillRect(16 - 7, 0, 9, 13, C_BLACK); // Believe its 7 pixels we need to blank, could be 8 - TBC
  RRE.drawChar(0, 2, CurrentMenuOption->Icon);
}

// Super simple display of basic centered text - no icons or anything (e.g. used for Device Name)
void Menus::DisplayMenuBasicCenteredText(char *text)
{
  ScrollMenuText = OFF; // Just in case any ongoing scrolling is happening!

  SetFontFixed();
  Display.fillRect(0, 0, SCREEN_WIDTH, 13, C_BLACK);
  RRE.printStr(ALIGN_CENTER, -1, text);
  SetFontCustom();
}

// Used for animated Menu line
// i.e. some form of visual indicator that we are in menu mode
int LineOffset = -16;
int LineWidth = 8;
int DoubleLineWidth = 16;

void Menus::Setup(Menu *rootMenu)
{
  // MenuFunctions::Setup();

  RootMenu = rootMenu; //&MainMenu;
  CurrentMenu = RootMenu;
}

// CONSOLIDATE REPEAT CODE IN 2 BELOW!

// Optimised to draw less when being used with main screen
void Menus::HandleMain()
{
  CurrentMenu->Handle();

  DisplayMenuTextOptimised();

  if (MenusStatus == ON)
  {
    if ((MenuFrame & 0x07) == 0)
    {
      // Used for animated Menu line
      LineOffset++;
      if (LineOffset == 0)
        // LineDirection = -LineDirection;
        LineOffset = -16;

      for (int i = LineOffset; i < SCREEN_WIDTH; i += DoubleLineWidth)
      {
        Display.drawFastHLine(i, 14, LineWidth, C_WHITE);
        Display.drawFastHLine(i + LineWidth, 14, LineWidth, C_BLACK);
      }
    }
  }

  MenuFrame++;
}

// Optimised to draw more when in config screen to allow for re-draws (where
// content might overwrite menu area)
void Menus::Handle_Config()
{
  CurrentMenu->Handle();

  DisplayMenuText();

  if (MenusStatus == ON)
  {
    if ((MenuFrame & 0x07) == 0)
    {
      // Used for animated Menu line
      LineOffset++;
      if (LineOffset == 0)
        // LineDirection = -LineDirection;
        LineOffset = -16;
    }

    for (int i = LineOffset; i < SCREEN_WIDTH; i += DoubleLineWidth)
    {
      Display.drawFastHLine(i, 14, LineWidth, C_WHITE);
      Display.drawFastHLine(i + LineWidth, 14, LineWidth, C_BLACK);
    }
  }

  MenuFrame++;
}

ControllerReport Menus::MoveUp()
{
  if (MenusStatus == OFF)
    return ReportToController;

  CurrentMenu->MoveUp();

  return DontReportToController;
}

ControllerReport Menus::MoveDown()
{
  if (MenusStatus == OFF)
    return ReportToController;

  CurrentMenu->MoveDown();

  return DontReportToController;
}

void Menus::Config_CheckForMenuChange()
{
  if (PRESSED == UpState() && UpJustChanged())
  //{
    CurrentMenu->MoveUp();
    //UpPressed = false;
  //}
  else if (PRESSED == DownState() && DownJustChanged())
  //{
    CurrentMenu->MoveDown();
    //DownPressed = false;
  //}
}

// Simplified check for digital inputs
// Simply set's flags if something is pressed
// It's up to individual menu options to work out how to handle. Could be different for different menu options!
void Menus::Config_CheckInputs()
{
  uint16_t state;
  Input *input;

  for (int i = 0; i < DigitalInputs_ConfigMenu_Count; i++)
  {
    input = DigitalInputs_ConfigMenu[i];

    input->ValueState.StateJustChanged = false;

    unsigned long timeCheck = micros();
    if ((timeCheck - input->ValueState.StateChangedWhen) > DEBOUNCE_DELAY)
    {
      // Compare current with previous, and timing, so we can de-bounce if required
      state = digitalRead(input->Pin);

      // Process when state has changed
      if (state != input->ValueState.Value)
      {
        Serial.println("Digital Input Changed: " + String(input->Label) + " to " + String(state));
        input->ValueState.PreviousValue = !state;
        input->ValueState.Value = state;
        input->ValueState.StateChangedWhen = timeCheck;
        input->ValueState.StateJustChanged = true;

        if (state == PRESSED)
        {
          // PRESSED!

          // Any extra special custom to specific controller code
          if (input->CustomOperationPressed != NONE)
            input->CustomOperationPressed();
        }
        else
        {
          // RELEASED!

          // Any extra special custom to specific controller code
          if (input->CustomOperationReleased != NONE)
            input->CustomOperationReleased();
        }
      } 
    }
  }
}