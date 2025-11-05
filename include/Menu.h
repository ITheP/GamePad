#pragma once

#include "config.h"

typedef struct MenuOption
{
    const char *Description;
    unsigned char Icon;
    char *Label;
    void (*InitOperation)();
    void (*Operation)();
    void (*ExitOperation)();
} MenuOption;

class Menu
{
public:
    // static int RequiredMenuOffset;
    MenuOption *MenuOptions;
    int MenuOptionsCount;

    MenuOption *CurrentMenuOption;
    int CurrentMenuOffset;
    int RequiredMenuOffset;

    // static int MenuMode;

    void Handle();

    void MoveUp();
    void MoveDown();
};