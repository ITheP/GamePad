#include "Battery.h"
#include "Structs.h"
#include "IconMappings.h"
#include "Screen.h"
#include "RenderText.h"
#include "Icons.h"
#include "DeviceConfig.h"
#include "Utils.h"
#include <Debug.h>

// 3.4 volts showing = starting but crashing @ 39%

extern CRGB ExternalLeds[];

int Battery::State = POWER_Battery;
int Battery::PreviousBatteryLevel = -1;
int Battery::ClampedBatterySensorReading = 0;
int Battery::ClampedBatteryPercentage = 0;
int Battery::CumulativeBatterySensorReadings = 0;
int Battery::BatteryLevelReadingsCount = 0;
int Battery::RawPowerSensorReading = 0;
float Battery::ClampedVoltage = 0.0;
float Battery::RawBatteryVoltage = 0.0;
float Battery::RawPinReading = 0.0; // Fractional as we averaging over several readings

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
    // return CurrentBatteryPercentage;
    return;
  }

  RawPinReading = CumulativeBatterySensorReadings / BatteryLevelReadingsCount;
  // We will clamp this reading lower down to effectively ignore any readings that are too high or too low, so we can easily have a 0% -> 100% reading without going outside this range in the UI
  // We don't care for ultimate accuracy - in device we are generally mapping to a small range of pixels in the UI, or a single decimal point accuracy for voltage
  CumulativeBatterySensorReadings = 0; // Ready for next round of readings
  BatteryLevelReadingsCount = 0;       // Set up to start readings again

  // Monitoring of power pin during testing generally showed it to be 4095 with occasional slight drop down 10-20, but was very rare.
  RawPowerSensorReading = analogRead(POWER_MONITOR_PIN);

  // Power present skews the reading (puts extra voltage on the battery pin when charging, or false voltage on battery pin if no battery)
  // so we artificially reduce the reading to estimate what actual battery is
  // Experiments showed around 34% more voltage than actual battery voltage when charging, and around 90% of actual battery voltage when no battery connected (but powered by USB)
  // We compensate by knocking off ~ 0.14 volts from the reading
  // All other estimates, ranges etc. should then work based on this adjusted `pretend` battery value
  if (RawPowerSensorReading > PWR_PRESENT_THRESHOLD)
  {
    RawPinReading -= ((float)(BATTERY_MAX - BATTERY_MIN) * 0.34); // Reduce reading by 34% of the range to estimate actual battery voltage);
  }

  RawBatteryVoltage = fmap(RawPinReading, BATTERY_MIN, BATTERY_MAX, BATTERY_MINV, BATTERY_MAXV);

  // ALTERNATIVE
  // Drain battery till things JUST fail
  // Measure manually battery charge at that point - that's our base line we dont want to go under (rather than just 3.3 being the bottom)
  // Set that as battery minimum
  // Charge battery till won't charge any more
  // Measure manually battery charge at that point - thats our top line we don't want to go over (rather than just 3.7 being the top)
  // FIND A BASE LINE READING (multimeter included) FOR LOWEST NON CRASHING BATTERY READING

  ClampedBatterySensorReading = RawPinReading;
  // Manual clamp for easy wrapping of serial information

  if (ClampedBatterySensorReading > BATTERY_MAX)
  {
#if defined(EXTRA_SERIAL_DEBUG)
    Serial.printf("🔋 ⚠️ Battery sensor reading was above the max value! Max: %d, Reading: %d\n", BAT_MAX, ClampedBatterySensorReading);
#endif
    ClampedBatterySensorReading = BATTERY_MAX;
  }
  else if (ClampedBatterySensorReading < BATTERY_MIN)
  {
#if defined(EXTRA_SERIAL_DEBUG)
    Serial.printf("🔋 ⚠️ Battery sensor reading was below the min! Min: %d, Reading: %d\n", BAT_MIN, ClampedBatterySensorReading);
#endif
    ClampedBatterySensorReading = BATTERY_MIN;
  }

  ClampedBatteryPercentage = fmap(ClampedBatterySensorReading, BATTERY_MIN, BATTERY_MAX, 0.0, 100.0);
  ClampedVoltage = fmap(ClampedBatterySensorReading, BATTERY_MIN, BATTERY_MAX, BATTERY_MINV, BATTERY_MAXV);

  // Work out if we are powered by battery, usb, or charging the battery
  // Note there is no `charging` state we can actually query, so we estimate based on
  // battery level and if we are powered by USB or not.
  // Assumption is
  // ...100% battery + USB power = usb powered
  // ...other battery + USB power = charging
  // ...else battery powered

  // float powerVoltage = (PowerSensorReading / ADC_RESOLUTION) * ADC_REF; // * PWR_DIVIDER_RATIO;

  if (RawPowerSensorReading > PWR_PRESENT_THRESHOLD)
  {
    if (ClampedBatteryPercentage == 100)
      State = POWER_USB; // Powered by USB, but battery is full, so not charging
    else
      State = POWER_Charging; // Powered by USB and battery is not full, so we are charging
  }
  else
    State = POWER_Battery; // Not powered by USB, so we are on battery

  Serial.printf("🔋RawPinReading: %.2f, PowerSensorReading: %d, RawVoltage: %f, Battery Reading: %d, Battery %: %d, Approx Battery Voltage: %.2f, State: %s\n",
                RawPinReading,
                RawPowerSensorReading,
                RawBatteryVoltage,
                ClampedBatterySensorReading,
                ClampedBatteryPercentage,
                ClampedVoltage,
                (State == POWER_Battery) ? "Battery" : (State == POWER_USB) ? "USB"
                                                                            : "Charging");

#ifdef EXTRA_SERIAL_DEBUG_PLUS
  Serial.println("Battery Sensor Limited: " + String(CurrentBatterySensorReading) + ", Battery %: " + String(CurrentBatteryPercentage) + ", Approx Battery Voltage: " + String(Voltage));
#endif
}

#define BatteryEmptyXPos ((SCREEN_WIDTH - 48) >> 1)
#define BatteryEmptyYPos ((SCREEN_HEIGHT - 16) >> 1)

IconRun BatteryEmptyGfx[] = {
    {.StartIcon = Icon_BatteryBigEmpty1, .Count = 3, .XPos = BatteryEmptyXPos, .YPos = BatteryEmptyYPos}};

int BatteryEmptyGfx_RunCount = sizeof(BatteryEmptyGfx) / sizeof(BatteryEmptyGfx[0]);

// Pass the exitInput here so we can display a label
void Battery::DrawFullDisplay(int secondRollover, int secondFlipFlop, bool canContinue, Input continueInput, bool includeLED)
{
  // Not very optimal drawing, but we don't care, we aren't doing anything else now
  bool isCharging = (Battery::State == POWER_Charging);

  Display.clearDisplay();

  int xPos = (SCREEN_WIDTH - 48) >> 1;

  // Draw big battery (includes empty gfx)
  RenderIconRuns(BatteryEmptyGfx, BatteryEmptyGfx_RunCount);

  // We only want the empty gfx if < 5%
  if (secondFlipFlop || ClampedBatteryPercentage >= POWER_EmptyPercentage)
  {
    // Hide middle bit of our battery warning intermittently to make it flash
    Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, 40, 12, C_BLACK);
  }

  // IF WE ARE CHARGING it looks like we are (certainly on this device)
  // around 0.136 volts lower than the actual reading (could be even less, need to run out battery and check level) when we are getting that 35% reading
  // Make this a device #define for an offset?
  // ALSO if plugged in and battery is not connected we get a 90% reading
  // on the battery pin it seems

  if (isCharging)
    snprintf(buffer, sizeof(buffer), "Charging... %d%%", ClampedBatteryPercentage);
  else
    snprintf(buffer, sizeof(buffer), "Battery... %d%%", ClampedBatteryPercentage);
  RRESmall.printStr(ALIGN_CENTER, 0, buffer);

  // Draw text...
  if (canContinue)
    snprintf(buffer, sizeof(buffer), "Hold %s to continue", DIGITALINPUT_BATTERY_CONTINUE_LABEL);
  else
    snprintf(buffer, sizeof(buffer), "Charge to %d%% to continue", POWER_EmptyPercentage);

  RRESmall.printStr(ALIGN_CENTER, SCREEN_HEIGHT - FONT_SMALL_HEIGHT, buffer);

  // Fancy animation when charging, otherwise just shows battery percentage
  if (secondFlipFlop)
  {
    if (isCharging)
    {
      for (float f = 0.0; f < 1.0; f += 0.02)
      {
        // Draw percentage full battery bar, with a moving fill to indicate charging
        Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, 40, 12, C_BLACK);
        Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, (int)((40.0 * f * ClampedBatteryPercentage) / 100.0), 12, C_WHITE);
        Display.display();
        delay(15);
      }
    }
    else
    {
      // Just draw a percentage full battery bar, no animation
      Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, (int)((40.0 * ClampedBatteryPercentage) / 100.0), 12, C_WHITE);
      Display.display();
    }
  }

#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
  if (includeLED)
  {
    // ToDo: If LED's were turned on at time this was instigated, might remain on. Double check to make sure these are turned off. (Less power drain then)
    if (secondFlipFlop)
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

// Simplified check for digital inputs for when in battery boot state
// Simply set's flags if something is pressed
// It's up to battery boot code to work out how to handle.
// TODO: Code is effectively identical to the Config menu input check, so could be refactored to a single method
void Battery::CheckInputs()
{
  uint16_t state;
  Input *input;

  // Just in case these are used for digital input virtual pins
  for (int i = 0; i < AnalogInputs_Count; i++)
  {
    input = AnalogInputs[i];
    uint16_t analogState = 0;

    if (input->Pin != NONE)
      analogState = analogRead(input->Pin);

    // Set triggered state if above TriggerOnValue
    if (input->TriggerOnValue > 0 && analogState > input->TriggerOnValue)
    {

      Serial.println("Analog Input Set: " + String(input->Label));
      input->ValueState.Value = PRESSED;
    }
    else
    {
      // Serial.println("Analog Input Cleared: " + String(input->Label));
      input->ValueState.Value = NOT_PRESSED;
    }
  }

  for (int i = 0; i < DigitalInputs_ConfigMenu_Count; i++)
  {
    input = DigitalInputs_ConfigMenu[i];

    input->ValueState.StateJustChanged = false;

    unsigned long timeCheck = micros();
    if ((timeCheck - input->ValueState.StateChangedWhen) > DEBOUNCE_DELAY)
    {
      // Compare current with previous, and timing, so we can de-bounce if required
      state = digitalRead(input->Pin);

      // Also check virtual inputs
      if (state == NOT_PRESSED && input->VirtualPinInputs.size() > 0)
      {
        for (int j = 0; j < input->VirtualPinInputs.size(); j++)
        {
          if (input->VirtualPinInputs[j]->ValueState.Value == PRESSED)
          {
            // Serial.println("Virtual Digital Input Set: " + String(input->Label));
            state = PRESSED;
            break;
          }
        }
      }

      // Process when state has changed
      if (state != input->ValueState.Value)
      {
        Serial.println("Digital Input Changed: " + String(input->Label) + " to " + String(state));
        input->ValueState.PreviousValue = !state;
        input->ValueState.Value = state;
        input->ValueState.StateChangedWhen = timeCheck;
        input->ValueState.StateJustChanged = true;

        if (state == PRESSED)
        {
          // PRESSED!

          // Any extra special custom to specific controller code
          if (input->CustomOperationPressed != NONE)
            input->CustomOperationPressed();
        }
        else
        {
          // RELEASED!

          // Any extra special custom to specific controller code
          if (input->CustomOperationReleased != NONE)
            input->CustomOperationReleased();
        }
      }
    }
  }
}