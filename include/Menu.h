#pragma once

#include "Config.h"
#include "Defines.h"

typedef struct MenuOption
{
    const char *Description;
    unsigned char Icon;
    const char *Label;
    void (*InitOperation)();
    void (*Operation)();
    void (*ExitOperation)();
} MenuOption;

class Menu
{
public:
    MenuOption *MenuOptions;
    int MenuOptionsCount;

    MenuOption *CurrentMenuOption;
    int CurrentMenuOffset;
    int RequiredMenuOffset;

    void Handle();

    void MoveUp();
    void MoveDown();
};