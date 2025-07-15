#include "Structs.h"
#include "LED.h"
// #include "LEDConfig.h"
// #include "ControllerConfig.h"
#include "DeviceConfig.h"
#include "GamePad.h"
#include "LEDEffects.h"

#ifdef INCLUDE_BENCHMARKS_LED
#include "Benchmark.h"
Benchmark LEDBenchmark("PERF.LED");
#endif

CRGB StatusLed[1];

float StatusLED_R = 0;
float StatusLED_G = 0;
float StatusLED_B = 0;

TaskHandle_t UpdateExternalLEDsTask;
volatile int UpdateLEDs = false;

// =========================================================

void InitExternalLED(ExternalLEDConfig *config, CRGB *leds)
{
  // We effectively account for either 1 LED being specified (nice and simple) or an array of them being specified (for fancy connected effects)
  // but rather than basing single LED's on a 1 element array, we effectively have a single LED reference, and an array reference
  // and subsequent process/effects can use either the single reference, or start going through arrays

  if (config->LEDNumbers.empty())
  {
    // Create a single entry based off the 1 LED, just in case it gets used
    config->LEDNumbersCount = 1;
    config->LEDNumbers = {config->LEDNumber};
  }
  else
  {
    config->LEDNumbersCount = config->LEDNumbers.size();
    config->LEDNumber = config->LEDNumbers[0]; // Set LEDNumber to the first LED in the array of led's
  }

  config->ExternalLEDs.resize(config->LEDNumbersCount);
  for (int i = 0; i < config->LEDNumbersCount; i++)
  {
    config->ExternalLEDs[i] = &leds[config->LEDNumbers[i]];
  }

  //  Direct reference to first LED
  config->ExternalLED = &leds[config->LEDNumber];

  // So by this point we should have
  // config->LEDNumber = the single LED or the first LED in the array of led's
  // config->ExternalLED = the single LED or the first LED in the array of led's
  // config->LEDNumber = 1 if single LED, else how many LED's in array
  // config->ExternalLEDs = single LED if only 1 set up, or array of LED's if multiple set. First/Single LED should match the config->ExternalLED

  // Serial.println("Setting up ExternalLED: " + String(config->LEDNumber));

  // Just in case an effect wants it, pre-calculate a HSV from Primary Colour
  config->PrimaryHSV = rgb2hsv_approximate(config->PrimaryColour.Colour);
  //}
}

extern CRGB ExternalLeds[];
extern int ExternalLedsEnabled[];
extern int Second;

#ifdef INCLUDE_BENCHMARKS_LED
int LEDPreviousSecond;
#endif

void UpdateExternalLEDsLoop(float onboardFadeRate, uint8_t externalFadeRate)
{
  for (;;)
  {
    // We don't want to do this as fast as possible, so we throttle it back
    if (UpdateLEDs)
    {
#ifdef INCLUDE_BENCHMARKS_LED
      int secondRollover;
      secondRollover = (Second != LEDPreviousSecond);
      LEDPreviousSecond = Second;

      LEDBenchmark.Start("UpdateLEDs", secondRollover);
#endif

#ifdef USE_EXTERNAL_LED
      // If idle just finished, make sure all LED's fade out
      if (ControllerIdleJustUnset)
      {
        for (int i = 0; i < IdleLEDEffects_Count; i++)
        {
          ExternalLEDConfig *ledConfig = IdleLEDEffects[i];

          if (ledConfig != nullptr)
             GeneralArrayEffects::DisableAll(ledConfig, Now);
        }

        ControllerIdleJustUnset = false;
      }
#endif

#ifdef INCLUDE_BENCHMARKS_LED
      LEDBenchmark.Snapshot("Loop.MiscLEDEffects", secondRollover);
#endif

      // Digital handling
      for (int i = 0; i < DigitalInputs_Count; i++)
      {
        Input *input = DigitalInputs[i];
        ExternalLEDConfig *ledConfig = input->LEDConfig;

#ifdef USE_EXTERNAL_LED
        // TODO: Just make default on/off effect for setting/unsetting leds rather than special code here?

        if (ledConfig != nullptr)
        {
          if (input->ValueState.StateJustChangedLED)
          {
            // if (ledConfig != nullptr)
            //{
            if (input->ValueState.Value == LOW)
            {
              // Pressed
              ledConfig->StartTime = Now; // Start time LED was engaged, in case we want to apply to some effects
              ExternalLedsEnabled[ledConfig->LEDNumber] = ledConfig->PrimaryColour.Enabled;
              if (ledConfig->Effect == nullptr)
              {
                // Only bother to set press colour if not doing some fancy mode later
                ExternalLeds[ledConfig->LEDNumber] = ledConfig->PrimaryColour.Colour;
              }
            }
            else
            {
              // Released
              ledConfig->StartTime = Now; // Start time LED was engaged, in case we want to apply to some effects
              ExternalLedsEnabled[ledConfig->LEDNumber] = ledConfig->SecondaryColour.Enabled;
              if (ledConfig->Effect == nullptr)
              {
                // Only bother to set press colour if not doing some fancy mode later
                ExternalLeds[ledConfig->LEDNumber] = ledConfig->PrimaryColour.Colour;
              }
            }
            //}
          }

          if (ledConfig->Effect != nullptr && (ExternalLedsEnabled[ledConfig->LEDNumber] || ledConfig->RunEffectConstantly))
          {
            if (ledConfig->Effect != nullptr)
              ledConfig->Effect(input, Now);
          }

          input->ValueState.StateJustChangedLED = false; // Changed at end incase effect wants to use it first
        }
#endif

// Onboard/Status LED
// Always check if we want to set the status LED i.e. hold LED on rather than just on a press
// We don't just overwrite values incase we darken a previous brighter, so as a basic additive, we keep the brightest components
#ifdef STATUS_LED_COMBINE_INPUTS
        if (input->ValueState.Value == LOW && input->OnboardLED.Enabled)
        {
          if (input->OnboardLED.Colour.r > StatusLED_R)
          {
            StatusLED_R = (float)(input->OnboardLED.Colour.r);
          }
          if (input->OnboardLED.Colour.g > StatusLED_G)
          {
            StatusLED_G = (float)(input->OnboardLED.Colour.g);
          }
          if (input->OnboardLED.Colour.b > StatusLED_B)
          {
            StatusLED_B = (float)(input->OnboardLED.Colour.b);
          }
        }
#endif
      }

#ifdef INCLUDE_BENCHMARKS_LED
      LEDBenchmark.Snapshot("Loop.DigitalInputs", secondRollover);
#endif

// Analog effects
#ifdef USE_EXTERNAL_LED
      for (int i = 0; i < AnalogInputs_Count; i++)
      {
        Input *input = AnalogInputs[i];
        int val = input->ValueState.Value;
        // Different to Digital Inputs - the analog is always set on an ongoing basis, not just when somethings pressed/unpressed

        // For an analog input, we set LED colour based on ranged value
        // Could do numerous things here, such as
        // - whammy bar sets brightness of other LED colours
        // - Whammy bar sets % of hue value
        // - Whammy bar sets % brightness
        // - Whammy bar sets % between a start and end colour (could be black and colour to fade out)
        // Recommend that whammy has a lower and upper start/end - so any calculations are clipped
        // e.g. map(state, 0, 4095, -20, 275) then clip to FastLED 0->255 range and we have a decent range ignoring the extremes

        ExternalLEDConfig *ledConfig = input->LEDConfig;
        // int ledNumber = config->LEDNumber;

        if (ledConfig != nullptr)
        {
          if (ledConfig->Effect == nullptr)
          {
            // Default effect
            int amount = map(val, 0, 4095, -20, 275);
            amount = constrain(amount, 0, 255);
            ExternalLeds[ledConfig->LEDNumber] = blend(ledConfig->SecondaryColour.Colour, ledConfig->PrimaryColour.Colour, amount);
            ExternalLedsEnabled[ledConfig->LEDNumber] = ledConfig->PrimaryColour.Enabled;
          }
          else
          {
            ledConfig->Effect(input, Now);

            ExternalLedsEnabled[ledConfig->LEDNumber] = ledConfig->PrimaryColour.Enabled;
          }
        }
      }
#endif

#ifdef INCLUDE_BENCHMARKS_LED
      LEDBenchmark.Snapshot("Loop.AnalogInputs", secondRollover);
#endif

// Hat effects
#ifdef USE_EXTERNAL_LED
      for (int i = 0; i < HatInputs_Count; i++)
      {
        HatInput *hatInput = HatInputs[i];

        int16_t hatCurrentState = hatInput->ValueState.Value;
        ExternalLEDConfig *ledConfig = hatInput->LEDConfigs[hatCurrentState];

        if (hatInput->ValueState.StateJustChangedLED)
        {
          // Handle previous state's LED
          // As HAT's might have effects that span other HAT LED's (such as highlighting LED's next to the
          // main current one) we actually go through all HAT LED's and do the equivalent of unset them)
          int16_t hatPreviousState = hatInput->ValueState.PreviousValue;
          ExternalLEDConfig *previousLEDConfig = hatInput->LEDConfigs[hatPreviousState];
          if (previousLEDConfig != nullptr)
          {
            for (int j = 0; j < 9; j++)
            {
              ExternalLEDConfig *tmpLEDConfig = hatInput->LEDConfigs[j];

              if (tmpLEDConfig != nullptr)
              {
                if (previousLEDConfig->Effect == nullptr)
                  *(tmpLEDConfig->ExternalLED) = tmpLEDConfig->SecondaryColour.Colour;

                ExternalLedsEnabled[tmpLEDConfig->LEDNumber] = tmpLEDConfig->SecondaryColour.Enabled;
              }
            }
          }

          // Handle new state's LED
          if (ledConfig != nullptr)
          {
            if (ledConfig->Effect == nullptr)
              *(ledConfig->ExternalLED) = ledConfig->PrimaryColour.Colour;

            ExternalLedsEnabled[ledConfig->LEDNumber] = ledConfig->PrimaryColour.Enabled;
          }
        }

        if (ledConfig != nullptr && ledConfig->Effect != nullptr && (ExternalLedsEnabled[ledConfig->LEDNumber] || ledConfig->RunEffectConstantly))
          ledConfig->Effect(hatInput, Now);

        hatInput->ValueState.StateJustChangedLED = false; // Changed at end incase effect wants to use it first

        // Onboard colours
        LED *onboardLED = &(hatInput->OnboardLED[hatInput->ValueState.Value]);
        if (onboardLED->Enabled)
        {
          if (onboardLED->Colour.r > StatusLED_R)
          {
            StatusLED_R = (float)(onboardLED->Colour.r);
          }
          if (onboardLED->Colour.g > StatusLED_G)
          {
            StatusLED_G = (float)(onboardLED->Colour.g);
          }
          if (onboardLED->Colour.b > StatusLED_B)
          {
            StatusLED_B = (float)(onboardLED->Colour.b);
          }
        }
      }
#endif

#ifdef INCLUDE_BENCHMARKS_LED
      LEDBenchmark.Snapshot("Loop.HATInputs", secondRollover);
#endif

#ifdef USE_EXTERNAL_LED
      // Misc. always running LED effects (not dependant on inputs)
      for (int i = 0; i < MiscLEDEffects_Count; i++)
      {
        ExternalLEDConfig *ledConfig = MiscLEDEffects[i];

        if (ledConfig != nullptr && ledConfig->Effect != nullptr)
          ledConfig->Effect(ledConfig, Now);
      }
#endif

#ifdef INCLUDE_BENCHMARKS_LED
      LEDBenchmark.Snapshot("Loop.MiscLEDEffects", secondRollover);
#endif

#ifdef USE_EXTERNAL_LED
      // TODO: If idle just unset - need to unset all LEDs to be disabled
      // Must do this as FIRST thing before all other LED effects above are processed
      // if (ControllerIdleJustUnset)

      // Idle LED effects
      if (ControllerIdle)
      {
        for (int i = 0; i < IdleLEDEffects_Count; i++)
        {
          ExternalLEDConfig *ledConfig = IdleLEDEffects[i];

          if (ledConfig != nullptr && ledConfig->Effect != nullptr)
            ledConfig->Effect(ledConfig, Now);
        }
      }
#endif

#ifdef INCLUDE_BENCHMARKS_LED
      LEDBenchmark.Snapshot("Loop.MiscLEDEffects", secondRollover);
#endif

      // Copy around any LEDs that need duplicating
      // e.g. when you might have multiple physical LED's that you want to share the same value, such as a light ring where you want the whole thing lit up at multiple points
      for (int i = 0; i < LEDClones_Count; i++)
      {
        IntPair pair = LEDClones[i];
        ExternalLeds[pair.B] = ExternalLeds[pair.A];
        // Serial.print("A: " + String(ExternalLeds[pair.A].r) + "," + String(ExternalLeds[pair.A].g) + "," + String(ExternalLeds[pair.A].b));
        // Serial.println(" = B: " + String(ExternalLeds[pair.B].r) + "," + String(ExternalLeds[pair.B].g) + "," + String(ExternalLeds[pair.B].b));
      }

#ifdef INCLUDE_BENCHMARKS_LED
      LEDBenchmark.Snapshot("Loop.Clone", secondRollover);
#endif

      FastLED.show();
      UpdateLEDs = false;

#ifdef INCLUDE_BENCHMARKS_LED
      LEDBenchmark.Snapshot("Loop.FastLED.Show", secondRollover);
#endif

      // Fade everything ready for next loop - and of course things might get reset elsewhere in the mean time but we don't care

      // Fade the status LED (fractional values)

      StatusLED_R -= onboardFadeRate;
      if (StatusLED_R < 0)
        StatusLED_R = 0;

      StatusLED_G -= onboardFadeRate;
      if (StatusLED_G < 0)
        StatusLED_G = 0;

      StatusLED_B -= onboardFadeRate;
      if (StatusLED_B < 0)
        StatusLED_B = 0;

#ifdef USE_EXTERNAL_LED
      // Fade disabled External LED's - note this are integer calculations so must be enough fade to actually have a decrease, and fractional components not applied over many iterations can add up to bigger differences over time. So this isn't exact.

      // We don't want to do this as fast as possible, and too many small fractional changes to work nicely with FastLED's preferred uint8_t
      // so we throttle back (assumes is delayed enough that there will always be a high enough value to affect a decrease)
      CRGB *crgb;
      float r, g, b;
      // As External LED's are updated to a value sporadically on press, and held without repeatedly being set each frame, we only fade when an LED is set as being disabled (i.e. a nice fade to being off)
      // TODO: Fade to an off colour rather than black?

      for (int i = 0; i < ExternalLED_FadeCount; i++)
      {
        if (!ExternalLedsEnabled[i])
        {
          crgb = &ExternalLeds[i];
          crgb->fadeToBlackBy(externalFadeRate);
        }
      }
#endif

#ifdef INCLUDE_BENCHMARKS_LED
      LEDBenchmark.Snapshot("Loop.End", secondRollover);
#endif
    }

    taskYIELD();
  }
}