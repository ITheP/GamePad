#pragma once

class Profile
{
public:
    // Constructors
    Profile(int id, char icon, String description);

    int Id;
    char Icon;  // Not currently used - need some icons!
    String Description;

    String WiFi_Name;
    String WiFi_Password;
    String CustomDeviceName;

    void Save();
    void Load();

private:
    char *PrefsKey;
};