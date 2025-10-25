#include "Battery.h"
#include "Structs.h"
#include "IconMappings.h"
#include "Screen.h"
#include "RenderText.h"
#include "Icons.h"
#include "DeviceConfig.h"
#include "Utils.h"

extern CRGB ExternalLeds[];

int Battery::PreviousBatteryLevel = -1;
int Battery::CurrentBatterySensorReading = 0;
int Battery::CurrentBatteryPercentage = 0;
int Battery::CumulativeBatterySensorReadings = 0;
int Battery::BatteryLevelReadingsCount = 0;
float Battery::Voltage = 0.0;

int Battery::GetLevel()
{
  // We have had multiple readings, so average them out and update current battery level
  CurrentBatterySensorReading = CumulativeBatterySensorReadings / BatteryLevelReadingsCount;
  CumulativeBatterySensorReadings = 0; // Ready for next round of readings
  BatteryLevelReadingsCount = 0;       // Set up to start readings again

  if (CurrentBatterySensorReading > BAT_MAX)
    CurrentBatterySensorReading = BAT_MAX;
  else if (CurrentBatterySensorReading < BAT_MIN)
    CurrentBatterySensorReading = BAT_MIN;

  CurrentBatteryPercentage = (CurrentBatterySensorReading - BAT_MIN) * 100.0 / (BAT_MAX - BAT_MIN);

  Voltage = fmap(CurrentBatteryPercentage, 0.0, 100.0, BAT_MINV, BAT_MAXV);

#ifdef EXTRA_SERIAL_DEBUG_PLUS
  Serial.println("Battery Sensor Limited: " + String(CurrentBatterySensorReading) + ", Battery %: " + String(CurrentBatteryPercentage) + ", Approx Battery Voltage: " + String(Voltage));
#endif

  return CurrentBatteryPercentage;
}

#define BatteryEmptyXPos ((SCREEN_WIDTH - 48) >> 1)
#define BatteryEmptyYPos ((SCREEN_HEIGHT - 16) >> 1)

IconRun BatteryEmptyGfx[] = {
    {.StartIcon = Icon_BatteryBigEmpty1, .Count = 3, .XPos = BatteryEmptyXPos, .YPos = BatteryEmptyYPos}};

int BatteryEmptyGfx_RunCount = sizeof(BatteryEmptyGfx) / sizeof(BatteryEmptyGfx[0]);

void Battery::DrawEmpty(int secondRollover, int SecondFlipFlop, bool IncludeLED)
{
  // Not very optimal drawing, but we don't care, we aren't doing anything else now
  if (secondRollover == true)
  {
    Display.clearDisplay();
    SetFontCustom();

    int xPos = (SCREEN_WIDTH - 48) >> 1;
    // int yPos = (SCREEN_HEIGHT - 16) >> 1;

    // char c = Icon_BatteryBigEmpty1;

    RenderIconRuns(BatteryEmptyGfx, BatteryEmptyGfx_RunCount);
    // for (uint8_t i = 0; i < 3; i++) {
    //   RenderIcon((char)c, xPos, yPos, 0, 0);
    //   xPos += 16;
    //   c++;
    // }X

    if (SecondFlipFlop)
    {
      // Hide middle bit of our battery warning intermittently to make it flash
      // xPos = (SCREEN_WIDTH - 48) >> 1;
      Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, 40, 12, C_BLACK);
    }

    Display.display();

#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
    if (IncludeLED)
    {
      // ToDo: If LED's were turned on at time this instigated, might remain on. Double check to make sure these are turned off.
      if (SecondFlipFlop)
        StatusLed[0] = CRGB::Black;
      else
        StatusLed[0] = CRGB::Red;

#if defined(USE_EXTERNAL_LED) && defined(ExternalLED_StatusLED)
      // Always make sure the external status LED is updated too
      ExternalLeds[ExternalLED_StatusLED] = StatusLed[0];
#endif

      FastLED.show();
    }
#endif
  }
}
