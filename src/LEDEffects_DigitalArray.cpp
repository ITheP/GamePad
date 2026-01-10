#include "GamePad.h"
#include "Structs.h"
#include "LED.h"
#include "LEDEffects.h"

extern int ExternalLedsEnabled[];

// Rain
//
// Blends all LED's from Secondary colour to Primary colour along the array, Primary being the front moving point
// Every RATE tick it moves all the LED's along the array to the end
// Every loop - set's the LED to current colour
//
// Variant where Rain fades to other colour
// Variant where it enables/disables LED's for a fading trail?
// Reminder that array is probably not going to be in the autofade range
//
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

// Rain effect but colour of drop blends from primary to secondary colour
// Ignores Primary and Secondary colour enabled flags
// LEDConfig.RunEffectConstantly should be true if you want constantly live effect
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

// Sparkle
//
// Goes through and if == primary then set to secondary
//   if enabled then will just be instantly set
//   if disabled then will fade from secondary to nothing
// Sets random to primary
//
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