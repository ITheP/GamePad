#include "Battery.h"
#include "Structs.h"
#include "IconMappings.h"
#include "Screen.h"
#include "RenderText.h"
#include "Icons.h"
#include "DeviceConfig.h"
#include "Utils.h"
#include <Debug.h>

extern CRGB ExternalLeds[];

int Battery::State = POWER_Battery;
int Battery::PreviousBatteryLevel = -1;
int Battery::ClampedBatterySensorReading = 0;
int Battery::ClampedBatteryPercentage = 0;
int Battery::CumulativeBatterySensorReadings = 0;
int Battery::BatteryLevelReadingsCount = 0;
int Battery::PowerSensorReading = 0;
float Battery::ClampedVoltage = 0.0;
float Battery::RawVoltage = 0.0;

#if defined(USE_EXTERNAL_LED) && defined(ExternalLED_StatusLED)
// Forward declaration - will be defined in device config
enum class LEDStrip;
#endif

void Battery::CalculateState()
{
  // We have had multiple readings, so average them out and update current battery level
  if (BatteryLevelReadingsCount == 0)
  {
    // This may get called between a previous reading and next one
    // We like having a few averaged readings rather than just doing 1 here and returning that
    // so we put up with it and simply return last reading
    //return CurrentBatteryPercentage;
    return;
  }

  // We will clamp this reading lower down to effectively ignore any readings that are too high or too low, so we can easily have a 0% -> 100% reading without going outside this range in the UI
  ClampedBatterySensorReading = CumulativeBatterySensorReadings / BatteryLevelReadingsCount;

  CumulativeBatterySensorReadings = 0; // Ready for next round of readings
  BatteryLevelReadingsCount = 0;       // Set up to start readings again

  // Not clamped yet, still raw
  float adcVoltage = (ClampedBatterySensorReading / ADC_RESOLUTION) * ADC_REF;
  RawVoltage = adcVoltage * BAT_DIVIDER_RATIO;

  // Clamp things down
  if (ClampedBatterySensorReading > BAT_MAX)
  {
#if defined(EXTRA_SERIAL_DEBUG)
    Serial.printf("🔋 ⚠️ Battery sensor reading was above the max value! Max: %d, Reading: %d\n", BAT_MAX, CurrentBatterySensorReading);
#endif
    ClampedBatterySensorReading = BAT_MAX;
  }
  else if (ClampedBatterySensorReading < BAT_MIN)
  {
#if defined(EXTRA_SERIAL_DEBUG)
    Serial.printf("🔋 ⚠️ Battery sensor reading was below the min! Min: %d, Reading: %d\n", BAT_MIN, CurrentBatterySensorReading);
#endif
    ClampedBatterySensorReading = BAT_MIN;
  }

  ClampedBatteryPercentage = (ClampedBatterySensorReading - BAT_MIN) * 100.0 / (BAT_MAX - BAT_MIN);
  ClampedVoltage = fmap(ClampedBatteryPercentage, 0.0, 100.0, BAT_MINV, BAT_MAXV);

  // Work out if we are powered by battery, usb, or charging the battery
  // Note there is no `charging` state we can actually query, so we estimate based on
  // battery level and if we are powered by USB or not. May get it wrong.
  // Assumption is we are 100% battery + USB power = charging
  // Monitoring during testing generally showed it to be 4095 with occasional slight drop down 10-20, but was very rare.
  PowerSensorReading = analogRead(POWER_MONITOR_PIN);
  float powerVoltage = (PowerSensorReading / ADC_RESOLUTION) * ADC_REF; // * PWR_DIVIDER_RATIO;

  if (powerVoltage > PWR_PRESENT_THRESHOLD)
  {
    if (ClampedBatteryPercentage == 100)
      State = POWER_USB; // Powered by USB, but battery is full, so not charging
    else
      State = POWER_Charging; // Powered by USB and battery is not full, so we are charging
  }
  else
    State = POWER_Battery; // Not powered by USB, so we are on battery

#ifdef EXTRA_SERIAL_DEBUG_PLUS
  Serial.println("Battery Sensor Limited: " + String(CurrentBatterySensorReading) + ", Battery %: " + String(CurrentBatteryPercentage) + ", Approx Battery Voltage: " + String(Voltage));
#endif

  //return CurrentBatteryPercentage;
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

    int xPos = (SCREEN_WIDTH - 48) >> 1;

    RenderIconRuns(BatteryEmptyGfx, BatteryEmptyGfx_RunCount);

    if (SecondFlipFlop)
    {
      // Hide middle bit of our battery warning intermittently to make it flash
      Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, 40, 12, C_BLACK);
    }

    Display.display();

#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
    if (IncludeLED)
    {
      // ToDo: If LED's were turned on at time this was instigated, might remain on. Double check to make sure these are turned off. (Less power drain then)
      if (SecondFlipFlop)
        StatusLed[0] = CRGB::Black;
      else
        StatusLed[0] = CRGB::Red;

#if defined(USE_EXTERNAL_LED) && defined(ExternalLED_StatusLED)
      // Always make sure the external status LED is updated too
      ExternalLeds[(int)LEDStrip::Status] = StatusLed[0]; // ExternalLED_StatusLED] = StatusLed[0];
#endif

      FastLED.show();
    }
#endif
  }
}
