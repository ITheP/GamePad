#pragma once

#include "Profile.h"

#define MAX_PROFILES 6  // Somewhat arbitrary limit, but was in line with a default + 5 spare corresponding to 5 input button presses on boot

class Profiles
{
public:
    // Constructors
    static Profile *AllProfiles[MAX_PROFILES];

    static Profile *GetProfileById(int id);

    static void SaveAll();
    static void LoadAll();
};