#pragma once

#include "config.h"
#include "Menu.h"
#include "Screen.h"

#define MenuTextBufferSize 256
#define MaxCharsFitOnScreen ((SCREEN_WIDTH / 2) + 1) // Theoretical max number of chars on screen (1 pixel wide) + 1 pixel spacings +1 for /0

#define MENUS_NOSCROLLNEEDED 0
#define MENUS_SCROLLCHECK 1
#define MENUS_SCROLLDEFINITELYNEEDED 2

class Menus
{
public:
    static int MenusStatus;
    static Menu *RootMenu;
    static Menu *CurrentMenu;
    static char MenuTextBuffer[];

    static int ToggleMenuMode();
    //static int SelectionOff();

    static void InitMenuItemDisplay(char *label, char *text, int scrollStatus = MENUS_SCROLLCHECK);
    static void UpdateMenuText(char *text, int scrollStatus);
    static void DisplayMenuText();
    static void DisplayMenuBasicCenteredText(char *text);


    static void Setup();
    static void Handle();

    static int MoveUp();
    static int MoveDown();
};