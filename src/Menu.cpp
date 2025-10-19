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

extern CRGB ExternalLeds[];

#define MenuContentStartX 22

void Menu::Handle()
{
  if (CurrentMenuOffset != RequiredMenuOffset)
  {
    if (CurrentMenuOption->ExitOperation != nullptr)
      CurrentMenuOption->ExitOperation();

    CurrentMenuOption = &MenuOptions[RequiredMenuOffset];
    CurrentMenuOffset = RequiredMenuOffset;

    if (CurrentMenuOption->InitOperation != nullptr)
      CurrentMenuOption->InitOperation();
  }

  if (CurrentMenuOption->Operation != nullptr)
    CurrentMenuOption->Operation();
}

void Menu::MoveUp()
{
  RequiredMenuOffset++;
  if (RequiredMenuOffset >= MenuOptionsCount)
    RequiredMenuOffset = 0;
}

void Menu::MoveDown()
{
  RequiredMenuOffset--;
  if (RequiredMenuOffset < 0)
    RequiredMenuOffset = MenuOptionsCount - 1;
}