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

// WiFi Access Point Settings
// While viewing this menu, pressing Select + Up/Down changes the selected AP
// however the access point list could be changing constantly with refreshes so...
// - Don't change the list while select is pressed
// - The currently selected (i.e. active) access point should always be visualised as being selected
// - The currently selected access point name in list we are pointing to should stay in position
// - The middle of the list on screen (with a visualisation) is the currently selected access point
// - If the currently selected access point is not in the list (e.g. out of range) we show "Not Available" instead
// The list should have a status icon showing signal strength and if its selected etc.
// We rebuild the on screen list whenever the access point lists changes or we move selection up and down

std::vector<AccessPoint *> ConfigAccessPointList;

// Placeholder access point if saved access point is not available
AccessPoint UnavailableSavedAccessPoint = {
    "", // Name get's set dynamically
    "",
    0,
    WIFI_AUTH_OPEN,
    false,
    Icon_WiFi_LostSignal,
    "Unavailable"};

// Placeholder for last selected access point if it suddenly disappears but we still want to see an entry for it
AccessPoint LastSelectedAccessPoint = {
    "", // Name get's set dynamically
    "",
    0,
    WIFI_AUTH_OPEN,
    false,
    Icon_WiFi_LostSignal,
    "Unavailable"};

// No access point selected (e.g. nothing saved in prefs)
AccessPoint NoAccessPoint = {
    "...select access point",
    "",
    1,
    WIFI_AUTH_OPEN,
    false,
    Icon_WiFi_Disabled,
    "Unavailable"};

String LastUISelectedAccessPointName = "";
int SelectedAccessPointIndex = 0;

void UpdateConfigAccessPointList(int listMovement = 0)
{
    ConfigAccessPointList.clear();

    // Just incase called when no list is available
    if (Network::AccessPointList.size() == 0)
        return;

    // Rebuild list
    AccessPoint *lastUISelectdAccessPoint = nullptr;
    AccessPoint *savedAccessPoint = nullptr;

    for (auto &entry : Network::AccessPointList)
    {
        const String &ssid = entry.first; // the key
        AccessPoint *ap = entry.second;   // the value

        ConfigAccessPointList.push_back(ap);

        if (ssid == LastUISelectedAccessPointName)
            lastUISelectdAccessPoint = ap;

        if (ssid == MenuFunctions::Current_Profile->WiFi_Name)
        {
            savedAccessPoint = ap;
            ap->selected = true;
        }
        else
            ap->selected = false;
    }

    // See if we found a matching ASP
    if (lastUISelectdAccessPoint == nullptr)
    {
        // No match? Stick in an error entry
        if (LastUISelectedAccessPointName == "")
            ConfigAccessPointList.push_back(&NoAccessPoint);
        else
        {
            LastSelectedAccessPoint.ssid = LastUISelectedAccessPointName;
            ConfigAccessPointList.push_back(&LastSelectedAccessPoint);
        }
    }

    // Add in saved access point if missing (and isn't the missing last match from above)
    if (savedAccessPoint == nullptr && LastUISelectedAccessPointName != MenuFunctions::Current_Profile->WiFi_Name)
    {
        UnavailableSavedAccessPoint.ssid = MenuFunctions::Current_Profile->WiFi_Name;
        ConfigAccessPointList.push_back(&UnavailableSavedAccessPoint);
    }

    // So right now we should have a list of access points including the currently selected one and saved one added if either were missing
    // Now we sort the list

    // Sort by RSSI
    std::sort(ConfigAccessPointList.begin(), ConfigAccessPointList.end(),
              [](const AccessPoint *a, const AccessPoint *b)
              {
                  // Sort alphabetically
                  return a->ssid > b->ssid;
                  // Sort by signal strength
                  //return a->rssi > b->rssi;
              });

    // Find offset of currently selected access point
    SelectedAccessPointIndex = 0;
    if (LastUISelectedAccessPointName == "")
    {
        // No previously selected AP - find the NoAccessPoint entry
        for (int i = 0; i < ConfigAccessPointList.size(); i++)
        {
            if (ConfigAccessPointList[i] == &NoAccessPoint)
            {
                SelectedAccessPointIndex = i;
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < ConfigAccessPointList.size(); i++)
        {
            if (ConfigAccessPointList[i]->ssid == LastUISelectedAccessPointName)
            {
                SelectedAccessPointIndex = i;
                break;
            }
        }
    }

    // At this point we should have
    //   - a sorted access point list
    //   - including last selected access point - even if it's disappeared
    //   - the saved access point - even if it's not found
    //   - an index of the currently selected access point in the list (or default of 0)

    // Now move it if needs be
    if (listMovement != 0)
    {
        SelectedAccessPointIndex += listMovement;
        if (SelectedAccessPointIndex < 0)
            SelectedAccessPointIndex = ConfigAccessPointList.size() - 1;
        else if (SelectedAccessPointIndex >= ConfigAccessPointList.size())
            SelectedAccessPointIndex = 0;

        LastUISelectedAccessPointName = ConfigAccessPointList[SelectedAccessPointIndex]->ssid;

        // There are many ways we could handle the selected access point, but for now we just blat it back into the current profile
        // where it sits there as current access point
        // It won't be saved unless user goes to SaveSettings menu option
        MenuFunctions::Current_Profile->WiFi_Name = LastUISelectedAccessPointName;
        // ...and yes, that might mean the name is invalid/unknown

        Serial_INFO;
        Serial.println("Selected Access Point changed to: " + LastUISelectedAccessPointName);
    }
}

void MenuFunctions::Config_Init_WiFi_AccessPoint()
{
    Menus::InitMenuItemDisplay(true);

    Display.fillRect(0, MenuContentStartY - 2, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY + 2), C_BLACK); // clears a bit of extra space in case other menu displayed outside usual boundaries

    LastUISelectedAccessPointName = MenuFunctions::Current_Profile->WiFi_Name;

    // Refresh the access point list to ensure pointers are valid
    // This is critical when returning from other menus where WiFi scans may have updated Network::AccessPointList
    UpdateConfigAccessPointList();
}

void MenuFunctions::Config_Update_WiFi_AccessPoint()
{
    int showScrollIcons = false;

    int listMovement = 0;

    // if (SecondRollover)
    if (PRESSED == Menus::SelectState())
    {
        showScrollIcons = true;

        if (PRESSED == Menus::UpState() && Menus::UpJustChanged())
            listMovement = 1;
        else if (PRESSED == Menus::DownState() && Menus::DownJustChanged())
            listMovement = -1;
    }
    else
    {
        Menus::Config_CheckForMenuChange(); // Handle changing menu option
    }

    // Anytime a WiFi refresh completes, access point list will become stale for our ConfigAccessPointList
    // so we make sure it is up to date
    // When access point lists are updated in Networking then old access point objects are deleted and new ones created - we are also making sure that
    // UpdateConfigAccessPointList is called to rebuild our list accounting for any possible object changes
    UpdateConfigAccessPointList(listMovement);

    if (DisplayRollover)
        Config_Draw_WiFi_AccessPoint(showScrollIcons);
}

void MenuFunctions::Config_Draw_WiFi_AccessPoint(int showScrollIcons)
{
    Display.fillRect(0, MenuContentStartY, SCREEN_WIDTH, (SCREEN_HEIGHT - MenuContentStartY), C_BLACK);

    SetFontTiny();
    SetFontLineHeightTiny();

    ResetPrintDisplayLine(MenuContentStartY, 10 + 12);

    int apSize = ConfigAccessPointList.size();
    if (apSize == 0)
    {
        RRE.setColor(C_WHITE);
        RRE.printStr(ALIGN_CENTER, MenuContentStartY + 12, "Scanning");
        RRE.printStr(ALIGN_CENTER, MenuContentStartY + 22, "Please Wait...");
    }
    else
    {
        // Show up to 5 access points, centred around selected one
        // SelectedAccessPointIndex = currently selected, displayed 3 lines down
        int startIndex = SelectedAccessPointIndex - 2;
        int apIndex = ((startIndex % apSize) + apSize) % apSize; // Positive modulo start point

        for (int i = 0; i < 5; i++)
        {
            int index = apIndex++ % apSize;
            AccessPoint *ap = ConfigAccessPointList[index];

            if (!ap)
            {
                Serial_ERROR;
                Serial.println("ERROR: Null AccessPoint in ConfigAccessPointList at index " + String(index));
                continue;
            }

            // if signal == 0 then something wrong (e.g. can't find, disconnected)

            if (ap->rssi == 0)
                RRE.drawChar(10, TextYPos - 1, 'x');
            else if (ap->rssi < 0)
            {
                // Convert signal strength into a little bar 1->8 pixels wide
                int w;
                if (ap->rssi > -50)
                    w = 8;
                else if (ap->rssi < -75)
                    w = 1;
                else
                    w = 2 + ((ap->rssi + 75) * 7) / 25;

                if (w > 8)
                    w = 8;

                Display.fillRect(10, TextYPos, w, 7, C_WHITE);
                // Serial.printf("RSSI %s: %d -> %d\n", ap->ssid.c_str(), ap->rssi, w);
            }

            // Serial.printf("RSSI %s: %d\n", ap->ssid.c_str(), ap->rssi);

            // Don't forget, PrintDisplayLine alters TextYPos so make sure we call AFTER the above
            PrintDisplayLine((char *)ap->ssid.c_str());

            // sprintf(buffer, "%s (%d)", ap->ssid.c_str(), ap->rssi);
            // PrintDisplayLine(buffer);
        }

        SetFontFixed();
        SetFontLineHeightFixed();

        // selected item indicator
        SetFontCustom();
        // Base size of left/right arrow icons is 7x7 px
        static int middle = ((SCREEN_HEIGHT - MenuContentStartY - 7) / 2) + MenuContentStartY + 2;
        int iconOffset = (Menus::MenuFrame >> 2) % 3;
        RenderIcon(Icon_Arrow_Right + iconOffset, 0, middle, 7, 7);
    }

    SetFontFixed();
    SetFontLineHeightFixed();

    if (showScrollIcons)
        DrawScrollArrows();

    SetFontFixed();
}