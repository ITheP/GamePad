#include <Arduino.h>
#include "Profiles.h"
#include "Profile.h"
#include "Prefs.h"

Profile *CurrentProfile;

Profile *Profiles::AllProfiles[MAX_PROFILES] = {nullptr};

Profile *Profiles::LoadProfileById(int profileId)
{
    Profile *profile = new Profile(profileId, (int)('0' + profileId), (profileId == 0 ? "Default" : String(profileId)));
    profile->Load();
    return profile;
}

void Profiles::SetCurrentProfile(Profile *profile)
{
    CurrentProfile = profile;
}

// Assumes all profiles have been loaded
void Profiles::SetCurrentProfileFromId(int id)
{
    if (id >= 0 && id < MAX_PROFILES)
        CurrentProfile = AllProfiles[id];
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
        AllProfiles[i] = new Profile(i, (int)('0' + i), String(i));

    for (int i = 0; i < MAX_PROFILES; i++)
        AllProfiles[i]->Load();
}