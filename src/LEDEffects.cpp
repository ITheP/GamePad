#include "Structs.h"
#include "LED.h"
#include "LEDEffects.h"

extern int ExternalLedsEnabled[];

uint32_t rnd = random();

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

// Variant where Rain fades to other colour
// Variant where it enables/disables LED's for a fading trail?
// Reminder that array is probably not going to be in the autofade range

// Secondary colour enabled = false - effect fades out
// Secondary colour enabled = true - instant snap to second colour
// ...so no fade effect e.g. if black instantly turns off LED as it passes
// LEDConfig.RunEffectConstantly should be true if you want constantly live effect
void DigitalArrayEffects::Rain(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  if ((ledConfig->PreviousTime + ledConfig->Rate) < time)
  {
    ledConfig->PreviousTime += ledConfig->Rate;

    // Shift all existing LED's along the array
    for (int i = ledConfig->LEDNumbersCount; i > 0; i--)
    {
      if (*(ledConfig->ExternalLEDs[i - 1]) == ledConfig->PrimaryColour.Colour)
      {
        // shift any active LED's up a position (except for very last LED)
        if (i < ledConfig->LEDNumbersCount)
        {
          *(ledConfig->ExternalLEDs[i]) = ledConfig->PrimaryColour.Colour;
          ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = ledConfig->PrimaryColour.Enabled;
        }

        // De-escalate previous LED
        *(ledConfig->ExternalLEDs[i - 1]) = ledConfig->SecondaryColour.Colour;
        ExternalLedsEnabled[ledConfig->LEDNumbers[i - 1]] = ledConfig->SecondaryColour.Enabled;
      }
    }
  }

  // Only on presses, not releases
  if (input->ValueState.StateJustChangedLED && input->ValueState.Value == LOW)
  {
    *(ledConfig->ExternalLEDs[0]) = ledConfig->PrimaryColour.Colour;
    ExternalLedsEnabled[ledConfig->LEDNumbers[0]] = ledConfig->PrimaryColour.Enabled;
  }
}

// LEDConfig.RunEffectConstantly should be true if you want constantly live effect

// Rain effect but colour of drop blends from primary to secondary colour
// Ignores Primary and Secondary colour enabled flags
void DigitalArrayEffects::BlendedRain(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  if ((ledConfig->PreviousTime + ledConfig->Rate) < time)
  {
    ledConfig->PreviousTime += ledConfig->Rate;

    // Shift all existing LED's along the array
    for (int i = ledConfig->LEDNumbersCount; i > 0; i--)
    {
      if (ExternalLedsEnabled[ledConfig->LEDNumbers[i - 1]])
      {
        // shift any active LED's up a position (except for very last LED)
        if (i < ledConfig->LEDNumbersCount)
        {
          float amount = ((float)(i) / (float)(ledConfig->LEDNumbersCount)) * 255.0;
          CRGB colour = blend(ledConfig->PrimaryColour.Colour, ledConfig->SecondaryColour.Colour, amount);

          *(ledConfig->ExternalLEDs[i]) = colour;
          ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = ExternalLedsEnabled[ledConfig->LEDNumbers[i - 1]];
        }

        // De-escalate previous LED
        ExternalLedsEnabled[ledConfig->LEDNumbers[i - 1]] = false;
      }
    }
  }

  // Only on presses, not releases
  if (input->ValueState.StateJustChangedLED && input->ValueState.Value == LOW)
  {
    *(ledConfig->ExternalLEDs[0]) = ledConfig->PrimaryColour.Colour;
    ExternalLedsEnabled[ledConfig->LEDNumbers[0]] = true;
  }
}

// Sparkle ->
// Goes through and if == primary then set to secondary
//   if enabled then will just be instantly set
//   if disabled then will fade from secondary to nothing
// Sets random to primary

// Rate represents density - could link density to button press
// TODO: Analog with value mapped to density?
// Rate = 0 to 0xFFFF (65535) (how likely an LED is to sparkle each frame). 5000 = 50% chance

void DigitalArrayEffects::Sparkle(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  uint32_t chance = ledConfig->Chance;

  if ((ledConfig->PreviousTime + ledConfig->Rate) < time)
  {
    ledConfig->PreviousTime += ledConfig->Rate;

    rnd += (uint32_t)time;

    // Shift all existing LED's along the array
    for (int i = 0; i < ledConfig->LEDNumbersCount; i++)
    {
      // De-sparkle if previously lit
      if (*(ledConfig->ExternalLEDs[i]) == ledConfig->PrimaryColour.Colour)
      {
        *(ledConfig->ExternalLEDs[i]) = ledConfig->SecondaryColour.Colour;
        ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = ledConfig->SecondaryColour.Enabled;
      }
      else
      {
        if (input->ValueState.Value == LOW)
        {
          // Nice fast random
          rnd ^= rnd << 13;
          rnd ^= rnd >> 17;
          rnd ^= rnd << 5;
          if (chance > (rnd & 0xFFFF))
          {
            *(ledConfig->ExternalLEDs[i]) = ledConfig->PrimaryColour.Colour;
            ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = ledConfig->PrimaryColour.Enabled;
          }
        }
      }
    }
  }
}

// Ignores Primary and Secondary colour enabled flags
void DigitalArrayEffects::SparkleTimeHue(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  uint32_t chance = ledConfig->Chance;

  time = time * ledConfig->Rate;
  uint8_t amount = (uint8_t)time;
  CRGB colour = CHSV(amount, 255, 255);

  rnd += (uint32_t)time;

  // Shift all existing LED's along the array
  for (int i = 0; i < ledConfig->LEDNumbersCount; i++)
  {
    // De-sparkle if previously lit
    if (ExternalLedsEnabled[ledConfig->LEDNumbers[i]] == true)
    {
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;
    }
    else
    {
      if (input->ValueState.Value == LOW)
      {
        // Nice fast random
        rnd ^= rnd << 13;
        rnd ^= rnd >> 17;
        rnd ^= rnd << 5;
        if (chance > (rnd & 0xFFFF))
        {
          *(ledConfig->ExternalLEDs[i]) = colour;
          ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true;
        }
      }
    }
  }
}

// Ignores Primary and Secondary colour enabled flags
void DigitalArrayEffects::SparkleTimeBlend(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  uint32_t chance = ledConfig->Chance;

  time = time * ledConfig->Rate;
  uint8_t amount = (uint8_t)time;
  amount = sin8(amount); // Smooth off when number wraps
  CRGB colour = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);

  rnd += (uint32_t)time;

  // Shift all existing LED's along the array
  for (int i = 0; i < ledConfig->LEDNumbersCount; i++)
  {
    // De-sparkle if previously lit
    if (ExternalLedsEnabled[ledConfig->LEDNumbers[i]] == true)
    {
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;
    }
    else
    {
      if (input->ValueState.Value == LOW)
      {
        // Nice fast random
        rnd ^= rnd << 13;
        rnd ^= rnd >> 17;
        rnd ^= rnd << 5;
        if (chance > (rnd & 0xFFFF))
        {
          *(ledConfig->ExternalLEDs[i]) = colour;
          ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true;
        }
      }
    }
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
    for (; i < tailEnd; i++) {
      *(ledConfig->ExternalLEDs[i]) = ledConfig->SecondaryColour.Colour;
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true;
    }

    // Front
    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true;
    *(ledConfig->ExternalLEDs[i++]) = ledConfig->PrimaryColour.Colour;
  }

  // Clear remaining LED's
  for (; i < count; i++)
    //*(ledConfig->ExternalLEDs[i]) = CRGB::Black;
    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;
}

// ===============================================================
// General effects - not linked to specific input
// e.g. Idle effects (when controllers getting bored)
// e.g. Stats effects - showing how many times a button or combination of buttons were pressed

// Ideally when LED's are turned off, just disable them rather than
// setting to e.g. black, else they won't fade out nicely

// Requires LED effect to have statistics set up
// Note we attach stats via LED configuration setting rather than the digital input
void GeneralArrayEffects::Elastic(void *externalLEDConfig, float time)
{
  ExternalLEDConfig *ledConfig = (ExternalLEDConfig *)externalLEDConfig;

  // Want access to elastic statistics
  Stats *stats = ledConfig->EffectStats;

  float perc = stats->ElasticPercentage * 255.0;

  for (int i = 0; i < ledConfig->LEDNumbersCount; i++)
  {
    float amount = ((float)(i) / (float)(ledConfig->LEDNumbersCount)) * 255.0;

    if (amount < perc) {
      *(ledConfig->ExternalLEDs[i]) = blend(ledConfig->PrimaryColour.Colour, ledConfig->SecondaryColour.Colour, amount);     
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true; // Could always get disabled by other effects
    }
    else {
      //colour = CRGB::Black;
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false; // Could always get disabled by other effects
    }
  }
}

uint32_t seed = 0x89ADCBEF;
// Random colours, ignores Primary and Secondary colour enabled flags
// Is written so the randomness is throttled - i.e. doesn't change every frame, but slower
// so you get to see random colours appearing and disappearing in a calmer way
void GeneralArrayEffects::Random(void *externalLEDConfig, float time)
{
  ExternalLEDConfig *ledConfig = (ExternalLEDConfig *)externalLEDConfig;

  uint32_t chance = ledConfig->Chance;

  uint32_t repRnd = seed * (uint32_t)(time * 2.5);    // Should change around 4 times a second

  CRGB colour;

  // rnd += (uint32_t)time;
  for (int i = 0; i < ledConfig->LEDNumbersCount; i++)
  {

    // Nice fast random
    repRnd ^= repRnd << 13;
    repRnd ^= repRnd >> 17;
    repRnd ^= repRnd << 5;

    if (chance > (repRnd & 0xFFFF))
    {
      //*(ledConfig->ExternalLEDs[i]) = CHSV(random8(), 255, 255);
      *(ledConfig->ExternalLEDs[i]) = CHSV(repRnd & 0xFF, 255, 255);
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true;
    }
    else
    {
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;
    }
  }
}

void GeneralArrayEffects::DisableAll(void *externalLEDConfig, float time)
{
  ExternalLEDConfig *ledConfig = (ExternalLEDConfig *)externalLEDConfig;

  for (int i = 0; i < ledConfig->LEDNumbersCount; i++)
    ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false;
}