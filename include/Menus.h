#pragma once

#include "Config.h"
#include "Defines.h"
#include "Menu.h"
#include "Screen.h"
#include "DeviceConfig.h"

#define MenuTextBufferSize 256
#define MaxCharsFitOnScreen ((SCREEN_WIDTH / 2) + 1) // Theoretical max number of chars on screen (1 pixel wide) + 1 pixel spacings +1 for /0

enum MenuScrollState
{
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

    static unsigned int MenuFrame;

    static ControllerReport ToggleMenuMode();

    static void InitMenuItemDisplay(int useMenuOptionLabel);
    static void InitMenuItemDisplay(char *text = NONE, MenuScrollState scrollStatus = NoScrollNeeded);
    static void UpdateMenuText(char *text, int scrollStatus);
    static void DisplayMenuTextOptimised();
    static void DisplayMenuText();
    static void DisplayMenuBasicCenteredText(char *text);

    static void Setup(Menu *menu);
    static void HandleMain();
    static void Handle_Config();

    static ControllerReport MoveUp();
    static ControllerReport MoveDown();

    static void Config_CheckInputs();
    static void Config_CheckForMenuChange();


    inline static int UpState()
    {
        return DigitalInput_Config_Up.ValueState.Value;
    }

    inline static int UpJustChanged()
    {
        return DigitalInput_Config_Up.ValueState.StateJustChanged;
    }

    inline static int DownState()
    {
        return DigitalInput_Config_Down.ValueState.Value;
    }

    inline static int DownJustChanged()
    {
        return DigitalInput_Config_Down.ValueState.StateJustChanged;
    }

    inline static int SelectState()
    {
        return DigitalInput_Config_Select.ValueState.Value;
    }

    inline static int SelectJustChanged()
    {
        return DigitalInput_Config_Select.ValueState.StateJustChanged;
    }

    inline static int BackState()
    {
        return DigitalInput_Config_Back.ValueState.Value;
    }

    inline static int BackJustChanged()
    {
        return DigitalInput_Config_Back.ValueState.StateJustChanged;
    }

private:
    static void RenderScrollingText();
    static void UpdateScrollingText();
    static void FinishScrollingText();
    static void DrawScrollingText();
    static void ReDrawScrollingText();
    static void DrawStaticMenuText();
    static void DrawMenuLine();
};