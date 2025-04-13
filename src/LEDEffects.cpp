#include "Structs.h"
#include "LED.h"
#include "LEDEffects.h"

// NOTES:
// FastLED does have faster routines than some of the below
// however our array's of LED's may be in any order (rather than continuous allocations of memory) and have other bits and bobs going on
// so our custom processing here does the job.
// May not be the fastest way of doing things, but does the job, and some routines are called from the LED centric Core 1 task keeping load out of the main loop.
// Calls are also throttled.

// FastLED loves its fract8 etc. datatype for fast floats - however some of the tests here where we were using fractions
// ended up with non-smooth transitions - so we float it!

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
    Serial.println(String(time) + ": Simple triggered");
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
    Serial.println(String(time) + ": Throb triggered");
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

// Blends all LED's from Secondary colour to Primary colour along the array, Primary being the front moving point
// Every RATE tick it moves all the LED's along the array to the end
// Every loop - set's the LED to current colour
float Rain_PreviousTime = 0;

// Variant where Rain fades to other colour
// Variant where it enables/disables LED's for a fading trail?
// Reminder that array is probably not going to be in the autofade range
void DigitalEffects::Rain(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  if ((Rain_PreviousTime + ledConfig->Rate) < time)
  {
    Rain_PreviousTime = time; //+= ledConfig->Rate;

    // Shift all existing LED's along the array
    for (int i = ledConfig->LEDNumbersCount - 1; i > 0; i--)
      *(ledConfig->ExternalLEDs[i]) = *(ledConfig->ExternalLEDs[i - 1]);

    *(ledConfig->ExternalLEDs[0]) = ledConfig->SecondaryColour.Colour;
  }

  // Only on presses, not releases
  if (input->ValueState.StateJustChangedLED && input->ValueState.Value == LOW)
  {
    *(ledConfig->ExternalLEDs[0]) = ledConfig->PrimaryColour.Colour;
  }
}

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

// ===============================================================

// TODO: ClipSet - at 50% value do one colour, else the other

void AnalogEffects::Test(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);
  ExternalLEDConfig *ledConfig = input->LEDConfig;

  // Serial.println(String(time) + ": Analog Test called");
}

// Note, values are mapped outside of final 0-255 range and then clipped to range - means tiny wobbles or variance at extremes should be stabalised
void AnalogEffects::SimpleSet(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275); // Map slightly outside 0-255 range
  amount = constrain(amount, 0, 255);                           // ...then clip to range
  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);
}

void AnalogEffects::Throb(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  if (input->ValueState.StateJustChangedLED)
  {
    input->ValueState.StateJustChangedLED = false;
    ledConfig->StartTime = time;
  }

  time = (time - ledConfig->StartTime) * ledConfig->Rate; // Rebase current time to LED's start timing * speed of throb
  uint8_t amount = (uint8_t)time;                         // 0->255
  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);
}

// Blended hue gradient, starting and ending at Primary colour, cross blended with Secondary colour. E.g. colour gradient faded with black
void AnalogEffects::BlendedHue(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275);
  amount = constrain(amount, 0, 255);
  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, CHSV(amount + ledConfig->PrimaryHSV.h, 255, 255), amount);
}

// Hue gradient, starting and ending at Primary colour
void AnalogEffects::Hue(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275);
  amount = constrain(amount, 0, 255);
  *(ledConfig->ExternalLED) = CHSV(amount + ledConfig->PrimaryHSV.h, 255, 255);
}

// Hue gradient, starting at Primary colour
void AnalogEffects::StartAtHue(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  // Colour = hue, offset by source colour
  int amount = map(input->ValueState.Value, 0, 4095, -20, 275);
  amount = constrain(amount, 0, 255);
  amount = map(amount, 0, 255, 0, 255 - 32);
  *(ledConfig->ExternalLED) = CHSV(amount + ledConfig->PrimaryHSV.h, 255, 255);
}

// Hue gradient, ending at Primary colour
void AnalogEffects::EndAtHue(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275);
  amount = constrain(amount, 0, 255);
  amount = map(amount, 0, 255, 32, 255);
  *(ledConfig->ExternalLED) = CHSV(amount + ledConfig->PrimaryHSV.h, 255, 255);
}

// Blended hue gradient, Secondary colour blended with hue, starting at Primary colour. E.g. colour gradient faded with black
void AnalogEffects::BlendedStartAtHue(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275);
  amount = constrain(amount, 0, 255);
  int remappedAmount = map(amount, 0, 255, 0, 255 - 32);
  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, CHSV(remappedAmount + ledConfig->PrimaryHSV.h, 255, 255), amount);
}

// Blended hue gradient, Secondary colour blended with hue, ending at Primary colour. E.g. colour gradient faded with black
void AnalogEffects::BlendedEndAtHue(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275);
  amount = constrain(amount, 0, 255);
  int remappedAmount = map(amount, 0, 255, 32, 255);
  //*(input->ExternalLED) = CHSV(amount + input->PrimaryHSV.h, 255, 255);
  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, CHSV(remappedAmount + ledConfig->PrimaryHSV.h, 255, 255), amount);
}

// ===============================================================

// Notes:
// .Value's are mapped outside of final 0-255 range and then clipped to range - means tiny wobbles or variance at extremes should be stabaliased

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
    *(ledConfig->ExternalLEDs[i]) = CRGB::Black;

  // work out how far along we want our colour to show. Range = 0->255 remapped to 0->count
  int along = map(amount, 0, 255, 0, count - 1);
  // Set that LED
  *(ledConfig->ExternalLEDs[along]) = blendedColour;
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
    *(ledConfig->ExternalLEDs[i]) = CRGB::Black;

  // work out how far along we want our colour to show. Range = 0->255 remapped to 0->count
  int along = map(amount, 0, 255, 0, count - 1);
  // Set that LED
  for (; along >= 0; along--)
  {
    *(ledConfig->ExternalLEDs[along]) = blendedColour;
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
    rampUp += rampUpRate;
  }

  // Clear remaining LED's
  for (; i < count; i++)
    *(ledConfig->ExternalLEDs[i]) = CRGB::Black;
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
    rampUp += rampUpRate;
  }

  // Clear remaining LED's
  for (; i < count; i++)
    *(ledConfig->ExternalLEDs[i]) = CRGB::Black;
}

// Blends all LED's from Secondary colour to Primary colour along the array, Primary being the front moving point
void AnalogArrayEffects::PointWithTail(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275); // Map slightly outside 0-255 range
  amount = constrain(amount, 0, 255);                           // ...then clip to range

  int count = ledConfig->LEDNumbersCount;

  // work out how far along we want our colour to show. Range = 0->255 remapped to 0->count
  int along = map(amount, 0, 255, 0, count);
  // This makes sure the 1st pixel is the PrimaryColour if only 1 pixel is shown
  int tailEnd = along - 1;

  int i = 0;
  if (amount > 0)
  {
    // Tail
    for (; i < tailEnd; i++)
      *(ledConfig->ExternalLEDs[i]) = ledConfig->SecondaryColour.Colour;

    // Front
    *(ledConfig->ExternalLEDs[i++]) = ledConfig->PrimaryColour.Colour;
  }

  // Clear remaining LED's
  for (; i < count; i++)
    *(ledConfig->ExternalLEDs[i]) = CRGB::Black;
}