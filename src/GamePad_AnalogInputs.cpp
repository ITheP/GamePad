
#include "GamePad_DigitalInputs.h"
#include <Structs.h>
#include <Debug.h>
#include <DeviceConfig.h>

extern CRGB ExternalLeds[];
extern int ExternalLedsEnabled[];

void setupAnalogInputs(Input *analogInputs[], int analogInputs_Count)
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

  // Basic Analog Inputs
  Serial.println();
  Serial_INFO;
  Serial.println("🎚️ Analog Inputs: " + String(analogInputs_Count));

  Input *input;
  for (int i = 0; i < analogInputs_Count; i++)
  {
    input = analogInputs[i];

    Serial.print("🎚️ " + String(input->Label));

    if (input->Pin != NONE)
    {
      Serial.print(", <- pin " + String(input->Pin));

      pinMode(input->Pin, INPUT);

      // analogSetPinAttenuation(input->Pin, input->analogAttenuation);
    }

    if (input->VirtualPinInputs.size() > 0)
    {
      for (int j = 0; j < input->VirtualPinInputs.size(); j++)
      {
        Serial.print(", <- Virtual Pin from [" + String(input->VirtualPinInputs[j]->Label) + "]");
      }
    }

    if (input->VirtualPulseInputs.size() > 0)
    {
      for (int j = 0; j < input->VirtualPulseInputs.size(); j++)
      {
        Serial.print(", <- Virtual Pulse Input from [" + String(input->VirtualPulseInputs[j].PulseInputSource->Label) + "]");
      }
    }

    input->ValueState.Value = input->DefaultValue;
    input->ValueState.AnalogValue = input->DefaultAnalogValue;

#ifdef USE_EXTERNAL_LED
    ExternalLEDConfig *config = input->LEDConfig;

    if (config != nullptr)
    {
      Serial.print(", External LED " + String(config->LEDNumber));
      InitExternalLED(config, ExternalLeds);
    }
    else
      Serial.print(", No LED");
#endif

    Serial.println();
  }
}