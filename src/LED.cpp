#include "Structs.h"
#include "LED.h"
// #include "LEDConfig.h"
// #include "ControllerConfig.h"
#include "DeviceConfig.h"
#include "GamePad.h"

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

void UpdateExternalLEDsLoop(float onboardFadeRate, uint8_t externalFadeRate)
{
  for (;;)
  {
    // We don't want to do this as fast as possible, so we throttle it back
    if (UpdateLEDs)
    {
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
            if (ledConfig != nullptr)
            {
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
            }
          }

          if (ledConfig->Effect != nullptr && ExternalLedsEnabled[ledConfig->LEDNumber])
          {
            if (ledConfig->Effect != nullptr)
              ledConfig->Effect(input, Now);
          }
                      
          // Serial.print("Digital State Just Changed: ");
          input->ValueState.StateJustChangedLED = false;
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

      // Copy around any LEDs that need duplicating
      // e.g. when you might have multiple physical LED's that you want to share the same value, such as a light ring where you want the whole thing lit up at multiple points
      for (int i = 0; i < LEDClones_Count; i++)
      {
        IntPair pair = LEDClones[i];
        ExternalLeds[pair.B] = ExternalLeds[pair.A];
        // Serial.print("A: " + String(ExternalLeds[pair.A].r) + "," + String(ExternalLeds[pair.A].g) + "," + String(ExternalLeds[pair.A].b));
        // Serial.println(" = B: " + String(ExternalLeds[pair.B].r) + "," + String(ExternalLeds[pair.B].g) + "," + String(ExternalLeds[pair.B].b));
      }

      FastLED.show();
      UpdateLEDs = false;

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
        crgb = &ExternalLeds[i];
        if (!ExternalLedsEnabled[i])
        {
          crgb->fadeToBlackBy(externalFadeRate);
        }
      }
#endif
    }

    taskYIELD();
  }
}