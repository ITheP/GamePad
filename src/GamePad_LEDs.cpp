#include "GamePad_LEDs.h"
#include <Structs.h>
#include <Debug.h>
#include <DeviceConfig.h>
#include "Screen.h"
#include "RenderText.h"
#include "Icons.h"
#include "LED.h"

#ifdef USE_EXTERNAL_LED

// extern CRGB ExternalLeds[ExternalLED_Count];
// extern int ExternalLedsEnabled[ExternalLED_Count];

void setupInitExternalLEDs()
{
  // Misc LEDs
  Serial.println("💡 Misc LED Effects: " + String(MiscLEDEffects_Count));
  for (int i = 0; i < MiscLEDEffects_Count; i++)
  {
    ExternalLEDConfig *config = MiscLEDEffects[i];

    if (config != nullptr)
    {
      Serial.print("MiscLEDEffects InitExternalLED: " + String(i) + ": ");
      InitExternalLED(config, ExternalLeds);
      Serial.println();
    }
  }

  // Idle LEDs
  for (int i = 0; i < IdleLEDEffects_Count; i++)
  {
    ExternalLEDConfig *config = IdleLEDEffects[i];

    if (config != nullptr)
    {
      InitExternalLED(config, ExternalLeds);
      Serial.println();
    }
  }
}
#endif

void setupLEDs()
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

  Serial.println();
  Serial_INFO;
  Serial.println("💡 Setting up LEDs...");

#ifdef USE_EXTERNAL_LED
  setupInitExternalLEDs();
#endif

  // Flash a little LED icon up to show we are playing with LED's
#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
  RREIcons.drawChar(112, 49, (unsigned char)Icon_LEDOn);
  Display.display();

  ApplyGlobalLEDBrightnessFromConfig();
  // FastLED.setBrightness(DEFAULT_LED_BRIGHTNESS);

  // FastLED.Show() main loop can be processed on a separate thread to allow for running on other cores.
  // At time of writing, thread runs on same core as main loop (core 1)
  // as Bluetooth/WiFi etc. run on core 0 and wan't to give them maximum performance there.
  // Gives us the option to play around a bit and choose where we want to run it in the future.

  // xTaskCreatePinnedToCore(
  //     UpdateExternalLEDs,
  //     "LEDUpdateTask",
  //     16000, // Stack size
  //     NULL,
  //     1,
  //     &UpdateExternalLEDsTask,
  //     1); // Core

  uint8_t hue = 0;

// Add and flash onboard led if enabled
#ifdef USE_ONBOARD_LED
  Serial.println();
  Serial.println("💡 LED pins\nOnboard: pin " + String(ONBOARD_LED_PIN));

  FastLED.addLeds<NEOPIXEL, ONBOARD_LED_PIN>(StatusLed, 1);
  // FastLED.addLeds<WS2812, ONBOARD_LED_PIN, GRB>(StatusLed, 1);

  for (hue = 0; hue < 255; hue++)
  {
    StatusLed[0] = CHSV(hue, 255, 255);
    FastLED.show();
    delay(3);
  }
  StatusLed[0] = CRGB::Black; // Next FastLED.show() will apply this
#endif

// Add and flash external led's if enabled
#ifdef USE_EXTERNAL_LED
  Serial.println();
  Serial.println("🔦 External: pin " + String(EXTERNAL_LED_PIN));

  FastLED.addLeds<EXTERNAL_LED_TYPE, EXTERNAL_LED_PIN, EXTERNAL_LED_COLOR_ORDER>(ExternalLeds, ExternalLED_FastLEDCount); // ExternalLED_Count);

  Serial.println("💡 " + String(ExternalLED_FastLEDCount) + " LEDs added, ExternalLED array length " + ExternalLED_Count);

  // Cycle through the external LED's as a pretty test
  hue = 0;

  // Flash LED's so no matter how many, they are shown within a small time frame.
  // Example of 5 leds using a delay of 30ms look nice and didn't take too long (150ms)
  // Scale this to total number in use and keep within same time frame
  int pause = 150 / ExternalLED_FastLEDCount;

  for (int cycles = 0; cycles < 4; cycles++)
  {
    RenderIcon(Icon_LEDOn, 112, 49, 16, 16);
    Display.display();

    for (int i = 0; i < ExternalLED_FastLEDCount; i++)
    {
      ExternalLeds[i] = CHSV(hue, 255, 255);
      FastLED.show();
      hue += 10;
      delay(pause);
      ExternalLeds[i] = CRGB::Black;
    }

    RenderIcon(Icon_LEDOff, 112, 49, 16, 16);
    Display.display();

    for (int i = ExternalLED_FastLEDCount - 1; i >= 0; i--)
    {
      ExternalLeds[i] = CHSV(hue, 255, 255);
      FastLED.show();
      hue += 10;
      delay(pause);
      ExternalLeds[i] = CRGB::Black;
    }
  }
  FastLED.show(); // Make sure final Black is accounted for

#endif // USE_EXTERNAL_LED
  // Final confirmation that either onboard and/or external LED's being used
  RREIcons.drawChar(112, 49, (unsigned char)Icon_LEDOn);
  Display.display();

  delay(Config_StartupThrottle.Value);
#else
  // No LED's being used!
  RREIcon.drawChar(112, 49, (unsigned char)Icon_LEDNone);
  Display.display();

  delay(Config_StartupThrottle.Value);
#endif
}
