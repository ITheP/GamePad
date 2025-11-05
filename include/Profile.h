#pragma once

class Profile
{
public:
    // Constructors
    Profile(int id, char icon, char *description);

    int Id;
    char Icon;
    const char *Description;

    String WiFi_Name;
    String WiFi_Password;
    String CustomDeviceName;

    void Save();
    void Load();

private:
    char *PrefsKey;
};