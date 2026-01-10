#include "Structs.h"
#include "LED.h"
#include "LEDEffects.h"

extern int ExternalLedsEnabled[];

void DigitalEffects::Test(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);
  ExternalLEDConfig *ledConfig = input->LEDConfig;

  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;

    Serial.println(String(time) + ": Digital Test clicked");
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

void DigitalEffects::SimpleSet(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;

#ifdef EXTRA_SERIAL_DEBUG
    Serial.println(String(time) + ": Simple triggered");
#endif
  }

  ExternalLEDConfig *ledConfig = input->LEDConfig;
  *(ledConfig->ExternalLED) = ledConfig->PrimaryColour.Colour;
}

void DigitalEffects::Throb(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;

#ifdef EXTRA_SERIAL_DEBUG
    Serial.println(String(time) + ": Throb triggered");
#endif
  }

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  if (input->ValueState.StateJustChanged)
    ledConfig->StartTime = time;

  time = (time - ledConfig->StartTime) * ledConfig->Rate; // Rebase current time to LED's start timing * speed of throb
  uint8_t amount = (uint8_t)time;                         // 0->255

  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);
}

void DigitalEffects::MoveRainbow(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  // LEDCustomTag represents hue, and is changed here - we don't care about external time
  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;
    ledConfig->CustomTag += ledConfig->Rate;
  }

  uint8_t amount = (uint8_t)(ledConfig->CustomTag); // 0->255
  *(ledConfig->ExternalLED) = CHSV(amount, 255, 255);
}

void DigitalEffects::Pulse(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

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

void DigitalEffects::TimeHue(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;
    ledConfig->StartTime = time;
  }

  time = (time - ledConfig->StartTime) * ledConfig->Rate; // Rebase current time to LED's start timing * speed of change
  uint8_t amount = (uint8_t)time;                         // 0->255
  *(ledConfig->ExternalLED) = CHSV(amount, 255, 255);
}