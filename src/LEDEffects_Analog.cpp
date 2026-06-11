#include "Structs.h"
#include "LED.h"
#include "LEDEffects.h"

extern int ExternalLedsEnabled[];

// Analog .Value's are mapped outside of final 0-255 range and then clipped to range - means tiny wobbles or variance at extremes should be stabilised

// TODO: ClipSet - at 50% value do one colour, else the other

void AnalogEffects::Test(void *digitalInput, float time)
{
  Input *input = static_cast<Input *>(digitalInput);
  ExternalLEDConfig *ledConfig = input->LEDConfig;

  // Serial.println(String(time) + ": Analog Test called");
}

// Basic analog mapping from Primary to Secondary colour based on value. Nothing fancy.
// Note, values are mapped outside of final 0-255 range and then clipped to range - means tiny wobbles or variance at extremes should be stabilised
void AnalogEffects::SimpleSet(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  int amount = map(input->ValueState.Value, 0, 4095, -20, 275); // Map slightly outside 0-255 range
  amount = constrain(amount, 0, 255);                           // ...then clip to range
  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);
}

// Basic analog mapping from Primary to Secondary colour based on value based on Min/Max range set on input. Nothing fancy.
// Note, values are mapped outside of final 0-255 range and then clipped to range - means tiny wobbles or variance at extremes should be stabilised
void AnalogEffects::ConstrainedSimpleSet(void *analogInput, float time)
{
  Input *input = static_cast<Input *>(analogInput);

  ExternalLEDConfig *ledConfig = input->LEDConfig;

  //int amount = map(input->ValueState.Value, input->MinAnalogValue, input->MaxAnalogValue, 0, 255); // Map slightly outside 0-255 range
 int amount = map(input->ValueState.AnalogValue, 0, 4095, -20, 275); // Map slightly outside 0-255 range
 // amount = constrain(amount, 0, 255);                           // ...then clip to range
  *(ledConfig->ExternalLED) = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);
  Serial.println("ConstrainedSimpleSet: Value: " + String(input->ValueState.Value) + ", Analog: " + String(input->ValueState.AnalogValue) + ", Amount: " + String(amount) + " - Primary: " + String(ledConfig->PrimaryColour.Colour.r) + "," + String(ledConfig->PrimaryColour.Colour.g) + "," + String(ledConfig->PrimaryColour.Colour.b) + " - Secondary: " + String(ledConfig->SecondaryColour.Colour.r) + "," + String(ledConfig->SecondaryColour.Colour.g) + "," + String(ledConfig->SecondaryColour.Colour.b));  
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