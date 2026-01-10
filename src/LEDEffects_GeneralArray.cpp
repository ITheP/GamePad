#include "Structs.h"
#include "LED.h"
#include "LEDEffects.h"

extern int ExternalLedsEnabled[];

// ===============================================================
// General effects - not linked to specific input
// e.g. Idle effects (when controllers getting bored)
// e.g. Stats effects - showing how many times a button or combination of buttons were pressed

// Ideally when LED's are turned off, just disable them rather than
// setting to e.g. black, else they won't fade out nicely

// EASING FUNCTION
//        (k * x)
// (1 - a^       )
// ---------------
//        k
//  1 - a^
// ...
// a = 42
// k = -1
// Gives nice curve from x,y = 0 to x,y = 1
// with a higher acceleration level at the start

// https://www.desmos.com/calculator
// \frac{\left(1\ -\ a^{\left(k\ \cdot\ x\right)}\right)}{\left(1\ -\ a\ ^{k}\right)}

// a = 42
// k = -1
// val = (1 - pow(a, (k * x))) / (1 - pow(a, k));

// Requires LED effect to have statistics set up
// Note we attach stats via LED configuration setting rather than the digital input
void GeneralArrayEffects::Elastic(void *externalLEDConfig, float time)
{
  ExternalLEDConfig *ledConfig = (ExternalLEDConfig *)externalLEDConfig;

  // Want access to elastic statistics
  Stats *stats = ledConfig->EffectStats;

  // float perc = stats->ElasticPercentage;
  float perc = stats->Current_CrossSecondCount / stats->ElasticMax; // e.g. max 20
  if (perc < 0.0)
    perc = 0.0;
  else if (perc > 1.0)
    perc = 1.0;

  // Serial.print(perc);

  //  Serial.println(" -> " + String(perc));

  //   for (int b = 0; b < (int)(perc * 100.0); b++)
  //   {
  //       if (b % 10 == 0)
  //       {
  //         int tensDigit = (b / 10) % 10; // Extract the tens unit
  //         Serial.print(tensDigit);

  //       }
  //       else
  //       {
  //         Serial.print("-");
  //       }
  //   }
  //   Serial.println();
  // return;
  //   constexpr  float a = 69.0;
  //   constexpr  float k = -1.0;
  //   constexpr  float ak = (1.0f - pow(a, k));

  //   perc = (1 - pow(a, (k * perc))) / ak;

  perc *= 255.0;

  for (int i = 0; i < ledConfig->LEDNumbersCount; i++)
  {
    float amount = ((float)(i) / (float)(ledConfig->LEDNumbersCount)) * 255.0;

    if (amount < perc)
    {
      *(ledConfig->ExternalLEDs[i]) = blend(ledConfig->PrimaryColour.Colour, ledConfig->SecondaryColour.Colour, amount);
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = true; // Could always get disabled by other effects
    }
    else
    {
      // colour = CRGB::Black;
      ExternalLedsEnabled[ledConfig->LEDNumbers[i]] = false; // Could always get disabled by other effects
    }
  }
}

uint32_t seed = 0x89ADCBEF;
// Random colours, ignores Primary and Secondary colour enabled flags
// Is written so the randomness is throttled - i.e. doesn't change every frame, but slower
// so you get to see random colours appearing and disappearing in a calmer way
// ledConfig->Rate - speed of effect, smaller values slow down the effect. e.g. 2.5 gives around 1/4 of a second periods
// ledConfig->Chance - density of random colours appearing - e.g. (0.1 * 0xFFFF) looks quite nice
// ledConfig->CustomTag = speed of Hue drift for each colour - e.g. 200 will give a nice little colour drift/throb over part of a second
void GeneralArrayEffects::Random(void *externalLEDConfig, float time)
{
  ExternalLEDConfig *ledConfig = (ExternalLEDConfig *)externalLEDConfig;

  uint32_t chance = ledConfig->Chance;

  uint32_t repRnd = seed * (uint32_t)(time * ledConfig->Rate); // Should change around 4 times a second

  CRGB colour;

  float intPart;
  float fracPart = modf(time, &intPart);
  uint8_t hueOffset = static_cast<uint8_t>(ledConfig->CustomTag * fracPart);

  // uint8_t brightness = static_cast<uint8_t>(255.0 * fracPart);

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
      *(ledConfig->ExternalLEDs[i]) = CHSV((uint8_t)(repRnd & 0xFF) + hueOffset, 255, 255); // brightness);
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