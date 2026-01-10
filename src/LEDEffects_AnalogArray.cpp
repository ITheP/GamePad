#include "Structs.h"
#include "LED.h"
#include "LEDEffects.h"

extern int ExternalLedsEnabled[];

// Analog .Value's are mapped outside of final 0-255 range and then clipped to range - means tiny wobbles or variance at extremes should be stabilised

// Uses single blended colour, setting single LED at point along array
void AnalogArrayEffects::SetSingleAlongArray(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275); // Map slightly outside 0-255 range
  amount = constrain(amount, 0, 255);                           // ...then clip to range

  CRGB blendedColour = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);
  //*(ledConfig->ExternalLED) = blendedColour;

  // Fill all LED's with the blended colour
  int count = ledConfig->LEDNumbersCount;
  // % along = i / count
  float percentAlong;

  // Clear existing colours
  for (int i = 0; i < count; i++)
    //*(ledConfig->ExternalLEDs[i]) = CRGB::Black;
    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;

  // work out how far along we want our colour to show. Range = 0->255 remapped to 0->count
  int along = map(amount, 0, 255, 0, count - 1);
  // Set that LED
  *(ledConfig->ExternalLEDs[along]) = blendedColour;
  ExternalLedsEnabled[ledConfig->LEDNumbers[along]] = true;
}

// Fills all LED's up to a point with same blended colour
void AnalogArrayEffects::FillToPointAlongArray(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275); // Map slightly outside 0-255 range
  amount = constrain(amount, 0, 255);                           // ...then clip to range

  CRGB blendedColour = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);

  int count = ledConfig->LEDNumbersCount;

  // Clear existing colours
  for (int i = 0; i < count; i++)
    //*(ledConfig->ExternalLEDs[i]) = CRGB::Black;
    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;

  // work out how far along we want our colour to show. Range = 0->255 remapped to 0->count
  int along = map(amount, 0, 255, 0, count - 1);
  // Set that LED
  for (; along >= 0; along--)
  {
    *(ledConfig->ExternalLEDs[along]) = blendedColour;
    ExternalLedsEnabled[ledConfig->LEDNumbers[along]] = true;
  }
}

// Blends all LED's from Secondary colour to ramping up primary colour moving along the array
void AnalogArrayEffects::BuildBlendToEnd(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275); // Map slightly outside 0-255 range
  amount = constrain(amount, 0, 255);                           // ...then clip to range

  CRGB blendedColour = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);

  int count = ledConfig->LEDNumbersCount;

  // work out how far along we want our colour to show. Range = 0->255 remapped to 0->count
  int along = map(amount, 0, 255, 0, count); // E.g. Amount = 128, Count = 10 -> along = 5
  float rampUpRate = 0;
  float rampUp = 0;
  if (along > 1)
    rampUpRate = (255.0 / (along - 1));

  // Serial.println("Amount: " + String(amount) + ", Along: " + String(along) + ", RampUpRate: " + String(rampUpRate) + ", RampUp: " + String(rampUp));

  // Set that LED
  int i = 0;
  for (; i < along; i++)
  {
    *(ledConfig->ExternalLEDs[i]) = blend(ledConfig->SecondaryColour.Colour, blendedColour, (fract8)rampUp);
    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true;
    rampUp += rampUpRate;
  }

  // Clear remaining LED's
  for (; i < count; i++)
    //  *(ledConfig->ExternalLEDs[i]) = CRGB::Black;
    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;
}

// Blends all LED's from Secondary colour to Primary colour along the array, Primary being the front moving point
void AnalogArrayEffects::SquishBlendToPoint(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275); // Map slightly outside 0-255 range
  amount = constrain(amount, 0, 255);                           // ...then clip to range

  int count = ledConfig->LEDNumbersCount;

  // work out how far along we want our colour to show. Range = 0->255 remapped to 0->count
  int along = map(amount, 0, 255, 0, count);
  float rampUpRate = 0;
  float rampUp = 0;
  // This makes sure the 1st pixel is the PrimaryColour if only 1 pixel is shown
  if (along > 1)
    rampUpRate = (255.0 / (along - 1));
  else
    rampUp = 255;

  // Set that LED
  int i = 0;
  for (; i < along; i++)
  {
    *(ledConfig->ExternalLEDs[i]) = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, (fract8)rampUp);

    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true; // Could always get disabled by other effects
    rampUp += rampUpRate;
  }

  // Clear remaining LED's
  for (; i < count; i++)
    //*(ledConfig->ExternalLEDs[i]) = CRGB::Black;
    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;
}

// Blends all LED's from Secondary colour to Primary colour along the array, Primary being the front moving point
void AnalogArrayEffects::PointWithTail(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  // int amount = map(input->ValueState.Value, input->MinValue, input->MaxValue, -20, 275); // Map slightly outside 0-255 range
  // amount = constrain(amount, 0, 255);                           // ...then clip to range
  int16_t minValue = input->MinValue;
  int16_t maxValue = input->MaxValue;
  int amount = constrain(input->ValueState.Value, minValue, maxValue);
  amount = map(amount, minValue, maxValue, -10, 265); // Slight tweak to the extremes to make sure min value shows nothing and max engages just before controls max point is reached

  amount = constrain(amount, 0, 255);

  int count = ledConfig->LEDNumbersCount;

  // work out how far along we want our colour to show. Range = 0->255 remapped to 0->count
  int along = map(amount, 0, 255, 0, count);
  // This makes sure the 1st pixel is the PrimaryColour if only 1 pixel is shown
  int tailEnd = along - 1;

  int i = 0;
  // Serial.println(amount);
  if (amount > 0)
  {
    // Tail
    for (; i < tailEnd; i++)
    {
      *(ledConfig->ExternalLEDs[i]) = ledConfig->SecondaryColour.Colour;
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true;
    }

    // Front
    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true;
    *(ledConfig->ExternalLEDs[i++]) = ledConfig->PrimaryColour.Colour;
  }

  // Serial.println("Count " + String(count) + ", LEDNumber at 0: " + String(ledConfig->LEDNumbers[i]));

  // Clear remaining LED's
  for (; i < count; i++)
    *(ledConfig->ExternalLEDs[i]) = CRGB::Black;
  // ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;
  // TODO: FADE ABOVE NOT WORKING, 1ST LED STAYS ON. POSSIBLY DUE TO LED MIRRORING RESETTING SOME VALUE? NOT SURE.
}