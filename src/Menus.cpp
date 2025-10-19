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

#define MenuContentStartX 22

unsigned int MenuFrame = 0;

int Menus::MenusStatus = OFF;
Menu *Menus::RootMenu = NONE;
Menu *Menus::CurrentMenu = NONE;

// Having a default MenuOption that does nothing helps save us check for null reference on
// uninitialised CurrentMenuOption later on
MenuOption *NoMenuOption = new MenuOption{"None", NONE, NONE, NONE};
// MenuOption *Menu::CurrentMenuOption = DefaultMenuOption;
// int RequiredMenuOffset = 0;
// int CurrentMenuOffset = 0;

MenuOption MainMenuOptions[] = {
    {"Name", NONE, NONE, MenuFunctions::InitName, NONE, NONE}, // Name is first menu item we initialise to to make sure Device Name is drawn. Has own special rendering of text so we don't get menu icons etc. being drawn
    {"Battery", NONE, "Battery", MenuFunctions::InitBattery, MenuFunctions::UpdateBattery, NONE},
    // new MenuOption{"Stats", &Menu::InitStats, &Menu::UpdateStats, NONE},
    {"About", NONE, "About", MenuFunctions::InitAbout, NONE, NONE},
    {"FPS", NONE, "FPS", MenuFunctions::InitFPS, MenuFunctions::UpdateFPS, NONE},
    {"Web Server", NONE, "Web Server", MenuFunctions::InitWebServer, MenuFunctions::UpdateWebServer, NONE},
    {"WiFi", NONE, "WiFi", MenuFunctions::InitWiFi, MenuFunctions::UpdateWiFi, NONE}};

Menu MainMenu = {
    MainMenuOptions,
    sizeof(MainMenuOptions) / sizeof(MainMenuOptions[0]),
    NoMenuOption,
    -1,
    0};

// TODO
// Parent/Child references to menus
// Enter submenu / return to parent menu operations
// Value selection operations for menu items that have values to select from (make this a reusable other menus, e.g. true/false
// Breadcrumb trail display of menu hierarchy

// int Menu::MenuOptionsCount = sizeof(Menu::MenuOptions) / sizeof(Menu::MenuOptions[0]);

int Menus::ToggleMenuMode()
{
  if (MenusStatus == ON)
  {
    MenusStatus = OFF;

    // Clear menu dotted line animation
    Display.drawFastHLine(0, 14, SCREEN_WIDTH, C_BLACK);

    return REPORTTOCONTROLLER_YES;
  }

  MenusStatus = ON;
  return REPORTTOCONTROLLER_NO;
}

#define MaxCharsFitOnScreen ((SCREEN_WIDTH / 2) + 1) // Theoretical max number of chars on screen (1 pixel wide) + 1 pixel spacings +1 for /0
// int BufferCopyCharsLength = MaxCharsFitOnScreen;                      // Number of chars that fit on screen + 1 to account for partial offsets to the left;
char Menus::MenuTextBuffer[MenuTextBufferSize + 4]; // General use Menu display text buffer to render dynamic content into - +4 to make sure there is space for ' - \0'
// char *CurrentMenuText;

int ScrollMenuText = OFF;
int DisplayTextStart = 14;
int MenuTextLength = 0; // Length of buffer without duplicated segment for scrolling
int MenuTextCharPos = 0;
int MenuTextStartCharWidth = 0;
int MenuTextPixelPos = MenuTextStartCharWidth;

// Menu auto-scrolls text if too long to fit on screen

void Menus::InitMenuItemDisplay(char *label, char *text, int scrollStatus)
{
  // MenuTextBuffer should have been populated with relevant text before calling this function

  Serial.println(label);

  // WORK OUT ICONS OR LEFT MENU TEXT STUFF HERE
  Display.fillRect(0, 0, SCREEN_WIDTH, RREHeight_fixed_8x16, C_BLACK);
  SetFontFixed();
  RRE.printStr(0, 0, label);
  SetFontCustom();

  if (text != NONE)
    UpdateMenuText(text, scrollStatus);
}

// Call ... checkScrollNeeded = false, scrollDefinitelyNeeded = false if you KNOW text will fit on screen
// Call ... checkScrollNeeded = true,  scrollDefinitelyNeeded = false if you aren't too sure text will fit
// Call ... checkScrollNeeded = false, scrollDefinitelyNeeded = true  if you KNOW text will NOT fit on screen
// We do all this to try minimise amount of overhead checking for text fit to work out if we need to scroll or not - high overheads
void Menus::UpdateMenuText(char *text, int scrollStatus)
{
  // CurrentMenuText = text;
  // MenuTextLength = std::strlen(CurrentMenuText);

  SetFontFixed();

  if (scrollStatus == MENUS_SCROLLCHECK)
  {

    // Work out width of text to see if scrolling is needed
    // Optimised so stop once off screen
    // CurrentMenuText = text;

    int totalWidth = DisplayTextStart;

    for (int i = 0; i < MenuTextLength; ++i)
    {
      totalWidth += RRE.charWidth(text[i]) + 1; // +1 for character spacing
      if (totalWidth > SCREEN_WIDTH)
      {
        scrollStatus = MENUS_SCROLLDEFINITELYNEEDED;
        break;
      }
    }
  }

  if (scrollStatus == MENUS_SCROLLDEFINITELYNEEDED)
  {
    // For scrolling text, stick a " - " on to visually separate end of text from start on wrap
    // Already MenuTextBuffer we can just amend it. For other strings, won't have buffer
    // space at the end so we copy into the MenuTextBuffer.
    // Note that text could get used elsewhere (logs, serial output, web output)
    // so we don't go round sticking the separator on everything in advance as we might not want it.
    if (text == MenuTextBuffer)
      strcat(MenuTextBuffer, " - ");
    else
      sprintf(MenuTextBuffer, "%s - ", text);

    // Scroll that sucker!
    // Pad the end of the text buffer with the starting segment so we can scroll seamlessly
    // strncat(MenuTextBuffer, MenuTextBuffer, BufferCopyCharsLength);
    MenuTextCharPos = 0;
    MenuTextStartCharWidth = RRE.charWidth(MenuTextBuffer[0]);
    MenuTextPixelPos = MenuTextStartCharWidth;
    ScrollMenuText = ON;

    // CurrentMenuText = MenuTextBuffer;
    MenuTextLength = std::strlen(MenuTextBuffer);

    // Serial.println("UpdateMenuText + tooWide = " + String(scrollStatus));
    // Serial.println("End buffer: " + String(CurrentMenuText));

    Serial.println("SCROLL TEXT SETUP: " + String(MenuTextBuffer));
  }
  else
  {
    // No scrolling needed - display now
    ScrollMenuText = OFF;

    Serial.println("NO SCROLL TEXT: " + String(text));

    // Redraw static text
    Display.fillRect(DisplayTextStart, 0, SCREEN_WIDTH - DisplayTextStart, RREHeight_fixed_8x16, C_BLACK);
    RRE.printStr(ALIGN_RIGHT, 0, text);
  }

  SetFontCustom();
}

// Super simple display of basic centered text - no icons or anything (e.g. used for Device Name)
void Menus::DisplayMenuBasicCenteredText(char *text)
{
  ScrollMenuText = OFF; // Just in case any ongoing scrolling is happening!

  SetFontFixed();
  Display.fillRect(0, 0, SCREEN_WIDTH, RREHeight_fixed_8x16, C_BLACK);
  RRE.printStr(ALIGN_CENTER, 0, text);
  SetFontCustom();
}

void Menus::DisplayMenuText()
{
  // Throttle scrolling to every other frame
  if (ScrollMenuText == ON && (MenuFrame & 0x01) == 0)
  {
    // Scroll the text
    Display.fillRect(DisplayTextStart, 0, SCREEN_WIDTH - DisplayTextStart, RREHeight_fixed_8x16, C_BLACK);
    SetFontFixed();

    int currentX = MenuTextPixelPos;
    int maxX = MenuTextPixelPos + SCREEN_WIDTH - 24;
    int charWidth;

    int i = MenuTextCharPos;
    while (currentX < maxX) // MenuTextBuffer[i] != '\0'; i++)
    {
      RRE.drawChar(currentX, 0, MenuTextBuffer[i]);
      currentX += RRE.charWidth(MenuTextBuffer[i]) + 1; // +1 for character spacing

      i++;
      if (i >= MenuTextLength)
        i = 0;
      // if (CurrentMenuText[i] == '\0')
      //   i = 0;

      // if (currentX > maxX)
      //   break; // Stop if next char exceeds width
    }

    MenuTextPixelPos--;
    if (MenuTextPixelPos < 0)
    {
      MenuTextPixelPos += RRE.charWidth(MenuTextBuffer[MenuTextCharPos]);
      MenuTextCharPos++;

      if (MenuTextCharPos >= MenuTextLength)
        MenuTextCharPos = 0;
    }

    SetFontCustom();
  }
}

// int Menus::SelectionOff()
// {
//   MenuMode = OFF;

//   return REPORTTOCONTROLLER_NO;
// }

// char AboutDetails[128];
// int AboutDetailsLength = 0;
// int AboutDetailsCopyLength = 0;
// int aboutCharPos = 0;
// int startCharWidth;
// int aboutPixelPos = startCharWidth;
int LineOffset = -16;
// int LineDirection = 1;
int LineWidth = 8;
int DoubleLineWidth = 16;

void Menus::Setup()
{
  MenuFunctions::Setup();
  // AboutDetailsLength = snprintf(AboutDetails, sizeof(AboutDetails), "%s - Core %s - FW v%s - HW v%s - SW v%s - ",
  //                               ModelNumber, getBuildVersion(), FirmwareRevision, HardwareRevision, SoftwareRevision);

  // // Copy start of AboutDetails to end of itself so when scrolling we can overrun the end without having to wrap to the start
  // AboutDetailsCopyLength = (SCREEN_WIDTH / 8) + 1; // Number of chars that fit on screen + 1 to account for partial offsets to the left

  // // if (AboutDetailsLength + copyLength < sizeof(Menu::AboutDetails)) {
  // strncat(AboutDetails, AboutDetails, AboutDetailsCopyLength);

  // startCharWidth = RRE.charWidth(AboutDetails[aboutCharPos]);
  // } else {
  //  Serial.println("Not enough space to duplicate string segment.");
  // }
  RootMenu = &MainMenu;
  CurrentMenu = RootMenu;
}

void Menus::Handle()
{
  DisplayMenuText();

  CurrentMenu->Handle();

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

int Menus::MoveUp()
{
  if (MenusStatus == OFF)
    return REPORTTOCONTROLLER_YES;

  CurrentMenu->MoveUp();

  return REPORTTOCONTROLLER_NO;
}

int Menus::MoveDown()
{
  if (MenusStatus == OFF)
    return REPORTTOCONTROLLER_YES;

  CurrentMenu->MoveDown();

  return REPORTTOCONTROLLER_NO;
}
