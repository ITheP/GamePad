#include "GamePad_DigitalInputs.h"
#include <Structs.h>
#include <Debug.h>
#include <DeviceConfig.h>

extern CRGB ExternalLeds[];
extern int ExternalLedsEnabled[];

void setupDigitalInputs(Input *digitalInputs[], int digitalInputs_Count)
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif
  // Simple Digital Inputs / Buttons

  Serial.println();
  Serial_INFO;
  Serial.println("🔘 Button/Digital Inputs: " + String(digitalInputs_Count));

  Input *input;
  for (int i = 0; i < digitalInputs_Count; i++)
  {
    input = digitalInputs[i];

    Serial.print("🔘 " + String(input->Label));

    if (input->Pin != NONE)
    {
      Serial.print(", <- pin " + String(input->Pin));

      pinMode(input->Pin, INPUT_PULLUP);
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

    // Make sure any long press inputs (child inputs) have their parent set to help identify they are a child.
    // Needed to make sure child inputs are skipped in certain circumstances - they are processed via their parent.
    // LED handling etc. ignore this and process all inputs always - never know what might need to be shown!
    // It is somewhat up to the controller designer to work out how to make sure nothing
    // conflicts if things like inputs use the same LED's as each other and what effects are in place

    if (input->LongPressChildInput != nullptr)
      input->LongPressChildInput->LongPressParentInput = input;

#ifdef USE_EXTERNAL_LED
    ExternalLEDConfig *config = input->LEDConfig;

    if (config != nullptr)
    {
      Serial.print(", external LED " + String(config->LEDNumber));
      InitExternalLED(config, ExternalLeds);
    }
    else
      Serial.print(", No LED");
#endif

    Serial.println();
  }
}