#pragma once

#include "FastLED.h"
//#include "DeviceConfig.h"

struct Input;
struct HatInput;
struct State;

extern CRGB StatusLed[1];

extern float StatusLED_R;
extern float StatusLED_G;
extern float StatusLED_B;

extern TaskHandle_t UpdateExternalLEDsTask;
extern volatile int UpdateLEDs;

typedef struct LED {
  CRGB Colour;
  int Enabled;

  LED(CRGB colour = CRGB::Black, int enabled = false)
    : Colour(colour), Enabled(enabled) {}
} LED;

extern int LEDClones_Count;

typedef struct ExternalLEDConfig {
  // External LED settings - hooked up to custom Neopixel/equivalent LED string
  int LEDNumber;                                            // LED number when single LED is used
  std::vector<int> LEDNumbers;                              // Where multiple LED's are used - note first LED in array is copied into LEDNumber.
  LED PrimaryColour;                                        // Primary color, usually used as the colour when button pressed
  LED SecondaryColour;                                      // Secondary color, usually used as the colour e.g. to fade out from when button not pressed, or the colour to blend up from in e.g. analog effects
  typedef void (*EffectFunctionPointer)(void*, float);
  EffectFunctionPointer Effect;                             // Effect, either DigitalEffect::, HatEffect:: or AnalogEffect:: function pointer
  int RunEffectConstantly;
  float Rate;                                               // Rate of effect, default to 0.0. - will vary in use depending on the Effect
  //float Frequency;                                          // How often ocurrs
  uint32_t Chance;                                          // For relevant effect, chance of something ocuring where 0 = 0% and 0xFFFF = 100%
  CRGB* ExternalLED;                                        // Pointer to actual LED settings - note this may be shared if the LEDNumber is re-used across inputs
  CHSV PrimaryHSV;                                          // Some effects want a Hue etc. - caches a precalculate value of this based on PrimaryColour
  float StartTime;                                          // Start time of effect, used for some effects when calculating how long the effect has been active
  float CustomTag;                                          // Custom tag, used for some effects that need to remember an additional value across calls


  int LEDNumbersCount;
  std::vector<CRGB*> ExternalLEDs;
  float PreviousTime;
} ExternalLEDConfig;

// If LEDNumbers.Size() > 0 then set LEDNumber = LEDNumbers[0] and copy &ExternalLEDs[0] to ExternalLED


void InitExternalLED(ExternalLEDConfig* config, CRGB* leds);
void UpdateExternalLEDsLoop(float onboardFadeRate, uint8_t externalFadeRate);