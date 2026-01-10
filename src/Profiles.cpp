#include <Arduino.h>
#include "Profiles.h"
#include "Profile.h"
#include "Prefs.h"

Profile *Profiles::AllProfiles[MAX_PROFILES] = { nullptr };

Profile *Profiles::GetProfileById(int profileId)
{
    Profile *profile = new Profile(profileId, (int)('0'+ profileId), String(profileId));
    profile->Load();
    return profile;
}

void Profiles::SaveAll()
{
    Serial.println("Loading all profiles");
    
    for (int i = 0; i < MAX_PROFILES; i++)
        AllProfiles[i]->Save();
}

void Profiles::LoadAll()
{
    Serial.println("Loading all profiles");

    // Creating a profile instance sets up references to required keys for loading/saving from prefs

    AllProfiles[0] = new Profile(0, '0', "Default");

    for (int i = 1; i < MAX_PROFILES; i++)
        AllProfiles[i] = new Profile(i, (int)('0'+ i), String(i));

    for (int i = 0; i < MAX_PROFILES; i++)
        AllProfiles[i]->Load();
}