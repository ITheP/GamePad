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
#include <GamePad.h>
#include <Battery.h>
#include <Network.h>
#include <Idle.h>

// NOTE what seems like needlessly complicated optimisations here faffing around with buffers and screen width
// checks etc. to display menu items is all to cut down overheads.
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
    // Name is first menu item we initialise to to make sure Device Name is drawn, so appears as default screen information
    // even when menu's haven't been engaged yet.
    // Has own special rendering of text so we don't get menu icons etc. being drawn
    {"Name", NONE, NONE, MenuFunctions::InitName, NONE, NONE},

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

MenuOption ConfigMenuOptions[] = {
    {"Help", Icon_Menu_QuestionMark, "Help", MenuFunctions::Config_Init_Help, MenuFunctions::Config_Update_Help, NONE}, // Name is first menu item we initialise to to make sure Device Name is drawn. Has own special rendering of text so we don't get menu icons etc. being drawn
    {"Profile", Icon_Menu_Smile, "Profile", MenuFunctions::Config_Init_Profile, MenuFunctions::Config_Update_Profile, NONE},
    {"WiFi Access Point", Icon_Menu_WiFi, "WiFi Access Point", MenuFunctions::Config_Init_WiFi_AccessPoint, MenuFunctions::Config_Update_WiFi_AccessPoint, NONE},
    {"WiFi Password", Icon_Menu_Key, "WiFi Password", MenuFunctions::Config_Init_WiFi_Password, MenuFunctions::Config_Update_WiFi_Password, MenuFunctions::Config_Exit_WiFi_Password},
    {"Hotspot", Icon_Menu_WiFi, "Hotspot", MenuFunctions::Config_Init_Hotspot, MenuFunctions::Config_Update_Hotspot, MenuFunctions::Config_Exit_Hotspot},
    {"Save Config", Icon_Menu_Save, "Save Settings", MenuFunctions::Config_Init_SaveSettings, MenuFunctions::Config_Update_SaveSettings, NONE}};

Menu Menus::ConfigMenu = {
    ConfigMenuOptions,
    sizeof(ConfigMenuOptions) / sizeof(ConfigMenuOptions[0]),
    NoMenuOption,
    -1,
    0};

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
char Menus::MenuTextBuffer[MenuTextBufferSize + 4];  // General use Menu display text buffer to render dynamic content into - +4 to make sure there is space for additional ' - \0' text when making a wrap around scrolling text and amending these characters for visual clarity

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
  Display.fillRect(0, 0, MenuTextStartX - 1, 13, C_BLACK);

  RREIcon.drawChar(0, 2, CurrentMenuOption->Icon);

  if (text != NONE)
    UpdateMenuText(text, scrollStatus);
}

// Call ... checkScrollNeeded = false, scrollDefinitelyNeeded = false if you KNOW text will fit on screen
// Call ... checkScrollNeeded = true,  scrollDefinitelyNeeded = false if you aren't too sure text will fit
// Call ... checkScrollNeeded = false, scrollDefinitelyNeeded = true  if you KNOW text will NOT fit on screen
// We do all this to try minimise needless overhead checking for text fit to work out if we need to scroll or not
void Menus::UpdateMenuText(char *text, int scrollStatus)
{
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
      totalWidth += RREDefault.charWidth(c) + 1; // +1 for character spacing

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

  // Make sure MenuTextBuffer contains the text for redraws, scrolling, etc.
  if (text != MenuTextBuffer)
    strcpy(MenuTextBuffer, text); // Shouldn't need strncopy()

  if (scrollStatus == ScrollDefinitelyNeeded)
  {
    // For scrolling text, stick a " - " on to visually separate end of text from start for wrapping text.
    // If text is already in MenuTextBuffer we can just amend. For other strings, won't have buffer
    // space at the end so we copy into the MenuTextBuffer.
    // Note that specified text could get used elsewhere (logs, serial output, web output)
    // so we don't go sticking the extra separator text in advance elsewhere as we might not want it.

    strcat(MenuTextBuffer, " - ");

    MenuTextLength += 3; // include the " - " above

    // Scroll that sucker!
    MenuTextCharPos = 0;
    MenuTextStartCharWidth = -RREDefault.charWidth(MenuTextBuffer[0]);
    MenuTextPixelPos = 0;
    ScrollMenuText = ON;

    // Scrolling doesn't start time wise till after a e.g. 2.5 second delay, to give user chance to read initial text
    MenuStartTime = Now + MenuStartDelay;

    // Draw initial text, which will stick around till the MenuStartTime kicks in with actual scrolling updates
    DrawScrollingText();
  }
  else
  {
    // No scrolling needed
    ScrollMenuText = OFF;

    DrawStaticMenuText();
  }
}

void Menus::DrawStaticMenuText()
{
  Display.fillRect(DisplayTextStart, 0, SCREEN_WIDTH - DisplayTextStart, 13, C_BLACK);
  RREDefault.printStr(ALIGN_RIGHT, -1, MenuTextBuffer);
}

// Optimised to draw less when in Main screen
void Menus::DisplayMenuTextOptimised()
{
  // Throttle scrolling to every other frame
  if (ScrollMenuText == ON && (MenuFrame & 0x01) == 0 && (MenuStartTime < Now))
    DrawScrollingText();
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
    RREDefault.printStr(ALIGN_RIGHT, -1, MenuTextBuffer);
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
  Display.fillRect(DisplayTextStart, 0, SCREEN_WIDTH - DisplayTextStart, 13, C_BLACK);

  int currentX = DisplayTextStart + MenuTextPixelPos;
  int maxX = SCREEN_WIDTH;
  int charWidth;

  int i = MenuTextCharPos;
  while (currentX < maxX) // MenuTextBuffer[i] != '\0'; i++)
  {
    RREDefault.drawChar(currentX, -1, MenuTextBuffer[i]);
    currentX += RREDefault.charWidth(MenuTextBuffer[i]) + 1; // +1 for character spacing

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

    MenuTextStartCharWidth = -RREDefault.charWidth(MenuTextBuffer[MenuTextCharPos]);
  }
}

void Menus::FinishScrollingText()
{
  // After scrolling content, the left edge rendering will creep past where the left of the
  // menu is - so we re-draw this area to the left to keep everything neat
  Display.fillRect(16 - 7, 0, 9, 13, C_BLACK);
  RREIcon.drawChar(0, 2, CurrentMenuOption->Icon);
}

// Super simple display of basic centered text - no icons or anything (e.g. used for Device Name)
void Menus::DisplayMenuBasicCenteredText(char *text)
{
  ScrollMenuText = OFF; // Just in case any previous ongoing scrolling is happening!

  Display.fillRect(0, 0, SCREEN_WIDTH, 13, C_BLACK);
  RREDefault.printStr(ALIGN_CENTER, -1, text);
}

// Used for animated Menu line - a visual indicator that we are in menu mode
int LineOffset = -16;
int LineWidth = 8;
int DoubleLineWidth = 16;

void Menus::Setup(Menu *rootMenu)
{
  RootMenu = rootMenu;
  CurrentMenu = RootMenu;
}

void Menus::DrawMenuLine()
{
  if (MenusStatus == ON)
  {
    if ((MenuFrame & 0x07) == 0)
    {
      // Used for animated Menu line
      LineOffset++;
      if (LineOffset == 0)
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

// Optimised to draw less when being used with main screen
void Menus::HandleMain()
{
  CurrentMenu->Handle();

  DisplayMenuTextOptimised();

  DrawMenuLine();
}

// Optimised to draw more when in config screen to allow for re-draws (where
// content might overwrite menu area)
void Menus::Handle_Config()
{
  CurrentMenu->Handle();

  DisplayMenuText();

  DrawMenuLine();
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
    CurrentMenu->MoveUp();
  else if (PRESSED == DownState() && DownJustChanged())
    CurrentMenu->MoveDown();
}

// Simplified check for digital inputs for when in Config menu
// Simply set's flags if something is pressed
// It's up to individual menu options to work out how to handle.
// Could be different for different menu options!
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