#include "Structs.h"
#include "LED.h"
#include "LEDEffects.h"

extern int ExternalLedsEnabled[];

// ===============================================================
// Hat effects are effectively the same as digital effects, but we do have the capacity to treat them differently
// e.g. if you want to make an effect that lights the currently selected direction, and the 2 directional points to the side of it are lit up with a dimmer complimentary colour

void HatEffects::Test(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);
  ExternalLEDConfig *ledConfig = input->LEDConfig;

  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;

    Serial.println(String(time) + ": Hat Test clicked");
    if (ledConfig == 0 || ledConfig == nullptr)
      Serial.println("Error: LEDConfig is null!");
    else
    {
      if (ledConfig->ExternalLED == 0 || ledConfig->ExternalLED == nullptr)
        Serial.println("Error: ExternalLED is null!");
      else
      {
        Serial.println("LED Number: " + String(ledConfig->LEDNumber) + ", Rate: " + String(ledConfig->Rate));
        Serial.println("PrimaryColor: " + String(ledConfig->PrimaryColour.Colour.r) + ", " + String(ledConfig->PrimaryColour.Colour.g) + ", " + String(ledConfig->PrimaryColour.Colour.b) + ", Enabled: " + String(ledConfig->PrimaryColour.Enabled));
      }
    }
  }
}

void HatEffects::SimpleSet(void *hatInput, float time)
{
  HatInput *input = static_cast<HatInput *>(hatInput);

  ExternalLEDConfig *ledConfig = input->LEDConfigs[input->ValueState.Value];
  *(ledConfig->ExternalLED) = ledConfig->PrimaryColour.Colour;
}

void HatEffects::Throb(void *hatInput, float time)
{
  HatInput *input = static_cast<HatInput *>(hatInput);

  int offset = input->ValueState.Value;
  ExternalLEDConfig *ledConfig = input->LEDConfigs[offset];

  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;
    ledConfig->StartTime = time;
  }

  time = (time - ledConfig->StartTime) * ledConfig->Rate; // Rebase current time to LED's start timing * speed of throb
  uint8_t amount = (uint8_t)time;                         // 0->255

  // if (ExternalLED == nullptr) {
  //     Serial.println("Error: ExternalLED is null!");
  //     return;
  // }

  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);
}

void HatEffects::MoveRainbow(void *hatInput, float time)
{
  HatInput *input = static_cast<HatInput *>(hatInput);

  int offset = input->ValueState.Value;
  ExternalLEDConfig *ledConfig = input->LEDConfigs[offset];

  // LEDCustomTag represents hue, and is changed here - we don't care about external time
  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;
    ledConfig->CustomTag += ledConfig->Rate;
  }

  uint8_t amount = (uint8_t)(ledConfig->CustomTag); // 0->255
  *(ledConfig->ExternalLED) = CHSV(amount, 255, 255);
}

void HatEffects::Pulse(void *hatInput, float time)
{
  HatInput *input = static_cast<HatInput *>(hatInput);

  int offset = input->ValueState.Value;
  ExternalLEDConfig *ledConfig = input->LEDConfigs[offset];

  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;
    ledConfig->StartTime = time;
  }

  time = (time - ledConfig->StartTime) * ledConfig->Rate; // Rebase current time to LED's start timing * speed of Pulse
  uint8_t amount = (uint8_t)time;                         // 0->255
  amount = sin8(amount);
  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);
}

void HatEffects::TimeHue(void *hatInput, float time)
{
  HatInput *input = static_cast<HatInput *>(hatInput);

  int offset = input->ValueState.Value;
  ExternalLEDConfig *ledConfig = input->LEDConfigs[offset];

  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;
    ledConfig->StartTime = time;
  }

  time = (time - ledConfig->StartTime) * ledConfig->Rate; // Rebase current time to LED's start timing * speed of change
  uint8_t amount = (uint8_t)time;                         // 0->255
  *(ledConfig->ExternalLED) = CHSV(amount, 255, 255);
}
