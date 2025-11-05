#pragma once

#include "config.h"
#include "Menu.h"
#include "Screen.h"

#define MenuTextBufferSize 256
#define MaxCharsFitOnScreen ((SCREEN_WIDTH / 2) + 1) // Theoretical max number of chars on screen (1 pixel wide) + 1 pixel spacings +1 for /0

enum MenuScrollState {
    NoScrollNeeded,
    ScrollCheck,
    ScrollDefinitelyNeeded
};

class Menus
{
public:
    static MenuOption *CurrentMenuOption;
    
    static Menu MainMenu;
    static Menu ConfigMenu;

    static int MenusStatus;
    static Menu *RootMenu;
    static Menu *CurrentMenu;
    static char MenuTextBuffer[];

    static ControllerReport ToggleMenuMode();
    //static int SelectionOff();

    static void InitMenuItemDisplay(int useMenuOptionLabel);
    static void InitMenuItemDisplay(char *text = NONE, MenuScrollState scrollStatus = NoScrollNeeded);
    static void UpdateMenuText(char *text, int scrollStatus);
    static void DisplayMenuText();
    static void DisplayMenuBasicCenteredText(char *text);

    static void Setup(Menu *menu);
    static void Handle();

    static ControllerReport MoveUp();
    static ControllerReport MoveDown();

private:
    static void DrawScrollingText();
};