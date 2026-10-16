#include "GamePad_Profile.h"
#include <Structs.h>
#include <Debug.h>
#include <DeviceConfig.h>
#include "Screen.h"
#include "RenderText.h"
#include "Icons.h"
#include "LED.h"
#include <Profiles.h>
#include <Networking.h>

Input *ProfileOverrideInput = nullptr;
extern int ESPChipIdOffset;

void setupProfile()
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

  Serial.println();
  Serial_INFO;
  Serial.println("👤 Setting up Profile...");

  // Special checks to start-up Inputs that allow buttons to be held to switch between variants of Bluetooth Id's.
  // Allows 1 controller to pair separately to multiple hosts/devices and allow for multiple
  // hosts to use same physical controller but as separate controllers,
  // Along with holding separate WiFi details and configurations if required.
  int profileId = 0;

  // TODO:> ALSO TRIGGER FROM ANALOG (may be virtualised) INPUTS

  Input *input;

  // Check analog inputs for a digital analog value to check for a profile, and populate values for virtual pins for digital inputs if required
  for (int i = 0; i < AnalogInputs_Count; i++)
  {
    input = AnalogInputs[i];
    uint16_t analogState = 0;

    if (input->Pin != NONE)
      analogState = analogRead(input->Pin);

    // Set triggered state if above TriggerOnValue
    if (input->TriggerOnValue > 0 && analogState > input->TriggerOnValue)
    {
      input->ValueState.Value = PRESSED;

      // Check if this analog input itself has a ProfileId set
      if (input->ProfileId > 0)
      {
        profileId = input->ProfileId;
        ProfileOverrideInput = input;
        Serial.println("🔗 Bluetooth Id Override : " + String(profileId));
      }
    }
    else
    {
      input->ValueState.Value = NOT_PRESSED;
    }
  }

  // Nothing yet? Check digital inputs
  if (profileId == 0)
  {
    for (int i = 0; i < DigitalInputs_Count; i++)
    {
      input = DigitalInputs[i];
      if (input->ProfileId > 0)
      {
        uint16_t state = NOT_PRESSED;

        // Check physical pin
        if (input->Pin != NONE)
          state = digitalRead(input->Pin);

        // Also check virtual inputs
        if (state == NOT_PRESSED && input->VirtualPinInputs.size() > 0)
        {
          for (int j = 0; j < input->VirtualPinInputs.size(); j++)
          {
            if (input->VirtualPinInputs[j]->ValueState.Value == PRESSED)
            {
              state = PRESSED;
              break;
            }
          }
        }

        if (state == PRESSED)
        {
          profileId = input->ProfileId;
          ProfileOverrideInput = input;
          Serial.println("🔗 Bluetooth Id Override : " + String(profileId));
        }
      }
    }
  }

  // This point we should have 0 (default) or 1-5 for our offset
  ESPChipIdOffset = profileId;

  // Load profile
  auto profile = Profiles::LoadProfileById(profileId);
  Profiles::SetCurrentProfile(profile);
  // CurrentProfile should now have profile loaded into it!

  // Populate custom names
  // TODO: CUSTOM NAMES

  Serial_INFO;
  Serial.print("👤 Using Profile ");
  Serial.print(String(CurrentProfile->Id));
  if (CurrentProfile->Id == 0)
    Serial.print(" (Default)");
  Serial.println();

  // Populate WiFi details
  if (CurrentProfile->WiFi_Name.length() == 0)
  {
    Serial.println("👤 🛜 ⛔ No WiFi specified, WiFi functionality disabled");
    Networking::WiFiDisabled = true;
    Networking::WiFiStatus = const_cast<const char *>("WiFi Disabled (Not Configured)");
    Networking::WiFiCharacter = Icon_WiFi_Disabled;
  }
  else
    Networking::WiFiDisabled = false;

  if (CurrentProfile->WiFi_Password.length() == 0)
    Serial.println("👤 🛜 ⚠️ No WiFi password specified. This might be ok.");

  Networking::ssid = CurrentProfile->WiFi_Name.c_str();
  Networking::password = CurrentProfile->WiFi_Password.c_str();
}
