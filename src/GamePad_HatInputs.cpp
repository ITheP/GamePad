#include "GamePad_DigitalInputs.h"
#include <Structs.h>
#include <Debug.h>
#include <DeviceConfig.h>

extern CRGB ExternalLeds[];
extern int ExternalLedsEnabled[];

void setupHatInputs(HatInput *hatInputs[], int hatInputs_Count)
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

  // Hat inputs also known as e.g. DPads

  Serial.println();
  Serial_INFO;
  Serial.println("🎩 Hat Inputs: " + String(hatInputs_Count));

  HatInput *hatInput;
  for (int i = 0; i < hatInputs_Count; i++)
  {
    hatInput = hatInputs[i];

    Serial.print("🎩 Hat " + String(hatInput->Label) + ": pins");

    for (int j = 0; j < 4; j++)
    {
      Serial.print(" ");

      hatInput->IndividualStates[j] = NOT_PRESSED;
      hatInput->IndividualStateChangedWhen[j] = 0;

      uint8_t pin = hatInput->Pins[j];
      if (pin == 0)
      {
        Serial.print("X");
      }
      else
      {
        Serial.print(pin);
        pinMode(pin, INPUT_PULLUP);
      }
    }

    hatInput->ValueState.Value = hatInput->DefaultValue;
    Serial.println();

#ifdef USE_EXTERNAL_LED
    Serial.print("🎩 Hat " + String(hatInput->Label) + ": LEDs");

    for (int i = 0; i < 9; i++)
    {
      Serial.print(" ");

      ExternalLEDConfig *config = hatInput->LEDConfigs[i];
      if (config != nullptr)
      {
        Serial.print(config->LEDNumber);
        InitExternalLED(config, ExternalLeds);
      }
      else
        Serial.print("X");
    }

    Serial.println();
#endif
  }
}