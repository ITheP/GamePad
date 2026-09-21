#include "Battery.h"
#include "Structs.h"
#include "IconMappings.h"
#include "Screen.h"
#include "RenderText.h"
#include "Icons.h"
#include "DeviceConfig.h"
#include "Utils.h"
#include <Debug.h>

#define CLAMP(x, lo, hi) ((x < lo) ? lo : (x > hi) ? hi \
                                                   : x)

extern CRGB ExternalLeds[];

PowerState Battery::State = PowerState::Unknown;

int Battery::Raw_CumulativeReadings = 0;
int Battery::Raw_CumulativeCount = 0;
float Battery::Raw_Battery = 0.0; // Fractional as we averaging over several readings
int Battery::Raw_Power = 0;

int Battery::PreviousPercentage = -1;
int Battery::Percentage = 0;
int Battery::ClampedPercentage = 0;
float Battery::Voltage = 0;
float Battery::ClampedVoltage = 0;
float Battery::ActualVoltage = 0;

float Battery::Minimum = 0;
float Battery::Minimum_Voltage = 0;
float Battery::Maximum = 0;
float Battery::Maximum_Voltage = 0;

#if defined(USE_EXTERNAL_LED) && defined(ExternalLED_StatusLED)
// Forward declaration - will be defined in device config
enum class LEDStrip;
#endif

// Helper function to convert PowerState enum to a readable string using an array lookup
const char* Battery::GetPowerStateString(PowerState state)
{
    // Check if the state value is within the bounds of the array
    if ((int)state >= 0 && (int)state < sizeof(PowerStateNames) / sizeof(PowerStateNames[0]))
    {
        return PowerStateNames[(int)state];
    }
    return "???"; // Default fallback if the state is out of bounds
}

const char* Battery::StateDescription()
{
  return GetPowerStateString(State);
}

void Battery::CalculateState()
{
  // We have had multiple readings, so average them out and update current battery level
  if (Raw_CumulativeCount == 0)
  {
    // This may get called between a previous reading and next one, so may be nothing to use yet
    return;
  }

  Raw_Battery = Raw_CumulativeReadings / Raw_CumulativeCount;
  Raw_CumulativeReadings = 0; // Ready for next round of readings
  Raw_CumulativeCount = 0;    // Set up to start readings again

  // Calculating battery state can be a bit messy. Some of it is a bit of guess work and estimates, can depend on
  // device, individual battery, charging circuit, tolerances, which way the wind is blowing, device load, etc.
  // Voltage estimates have proved to be inaccurate in theory vs measured.
  // This version is based on a pin that measures battery level, and also connected power supply, so we can
  // estimate if we are charging or not, if we are powered by cable or by battery.
  // Note that having a power cable connected/charging alters the readings on the battery pin!
  // So we use the following...
  // - Ignoring documented conversion ratios for pin readings -> voltage conversions, we based our pin readings on physical multimeter readings
  // - Actual power readings of plugged in power supply aren't really important - just if it plugged in or not
  // - Voltage range on battery pin is different for battery only or plugged in at same time, so we use 2 voltage ranges to measure 0% -> 100% battery left
  // - As battery becomes full it becomes hard to distinguish between power only (no battery) and power + battery charging

  // We need to know the state of the power pin to know which range to use for the battery pin measurements,
  // as the battery pin is affected by the power pin (charging or not charging)
  Raw_Power = analogRead(POWER_MONITOR_PIN);

  PowerState state;
  float min;
  float min_v;
  float full;
  float full_v;
  float max;
  float max_v;

  if (Raw_Power < POWER_PRESENT)
  {
    min = BATTERY_MIN;
    min_v = BATTERY_MIN_V;
    full = BATTERY_FULL;
    full_v = BATTERY_FULL_V;
    max = BATTERY_MAX;
    max_v = BATTERY_MAX_V;

    if (Raw_Battery < full)
      state = PowerState::Battery;
    else
      state = PowerState::Battery_Full;
  }
  else
  {
    min = CHARGING_MIN;
    min_v = CHARGING_MIN_V;
    full = CHARGING_FULL;
    full_v = CHARGING_FULL_V;
    max = CHARGING_MAX;
    max_v = CHARGING_MAX_V;

    if ((Raw_Battery > BATTERY_CHECK) && (Raw_Power < POWER_MAX))
      state = PowerState::USB_Battery_Charging;
    else if ((Raw_Battery >= BATTERY_FULL) && (Raw_Power > POWER_PRESENT))
      state = PowerState::USB_Battery_Full;
    else if ((Raw_Battery < BATTERY_CHECK) && (Raw_Power == POWER_MAX))
      state = PowerState::USB;
    else
      state = PowerState::Unknown;
  }

  PreviousPercentage = Percentage;
  float percentage = fmap(Raw_Battery, min, full, 0.0, 100.0);  // Maybe > 100% (when overcharged)
  Percentage = (int)percentage;
  float clampedPercentage = (CLAMP(percentage, 0.0, 100.0));    // Clamped for nice easy UI usage
  ClampedPercentage = (int)clampedPercentage;

  // Voltage, based on % against battery range, which will remap powered range to battery range if required
  Voltage = fmap(percentage, 0, 100, BATTERY_MIN_V, BATTERY_FULL_V);  // May go > full %
  ClampedVoltage = (int)(CLAMP(clampedPercentage, BATTERY_MIN_V, BATTERY_FULL_V));
  // Voltage, based on actual voltage of battery or powered, no remapping
  ActualVoltage = fmap(Raw_Battery, min, max, min_v, max_v);

  Minimum = min;
  Minimum_Voltage = min_v;
  Maximum = max;
  Maximum_Voltage = max_v;

  // #ifdef EXTRA_SERIAL_DEBUG_PLUS
  Serial.printf("🔋 State: [%-22s] - RawBattery: %.2f, RawPower: %d, Percentage: %.2f%% -> %d%% -> %d%%, Voltage: %.2f, ClampedVoltage: %.2f, ActualVoltage: %.2f, Pin Range: %.2f->%.2f, Voltage Range: %.2f->%.2f, Bat: [%.2f->%.2f , %.2f->%.2f], Charging: [%.2f->%.2f , %.2f->%.2f], %.2f%%=low, %.2f%%=empty\n",
                GetPowerStateString(state),
                Raw_Battery,
                Raw_Power,
                percentage,
                Percentage,
                ClampedPercentage,
                Voltage,
                ClampedVoltage,
                ActualVoltage,
                Minimum,
                Maximum,
                Minimum_Voltage,
                Maximum_Voltage,
                (float)BATTERY_MIN,
                (float)BATTERY_MAX,
                (float)BATTERY_MIN_V,
                (float)BATTERY_MAX_V,
                (float)CHARGING_MIN,
                (float)CHARGING_MAX,
                (float)CHARGING_MIN_V,
                (float)CHARGING_MAX_V,
                (float)POWER_Percentage_Low,
                (float)POWER_Percentage_Empty);
//#endif
}

// void Battery::CalculateState()
// {
//   // We have had multiple readings, so average them out and update current battery level
//   if (BatteryLevelReadingsCount == 0)
//   {
//     // This may get called between a previous reading and next one
//     // We like having a few averaged readings rather than just doing 1 here and returning that
//     // so we put up with it and simply return last reading
//     // return CurrentBatteryPercentage;
//     return;
//   }

//   RawPinReading = CumulativeBatterySensorReadings / BatteryLevelReadingsCount;
//   // We will clamp this reading lower down to effectively ignore any readings that are too high or too low, so we can easily have a 0% -> 100% reading without going outside this range in the UI
//   // We don't care for ultimate accuracy - in device we are generally mapping to a small range of pixels in the UI, or a single decimal point accuracy for voltage
//   CumulativeBatterySensorReadings = 0; // Ready for next round of readings
//   BatteryLevelReadingsCount = 0;       // Set up to start readings again

// // Calculating battery state can be a bit messy. Some of it is a bit of guess work and estimates, can depend on
// // device, individual battery, charging circuit, tolerances, which way the wind is blowing, device load, etc.
// // Voltage estimates have proved to be inaccurate in theory vs measured.
// // This version is based on a pin that measures battery level, and also connected power supply, so we can
// // estimate if we are charging or not, if we are powered by cable or by battery.
// // Note that having a power cable connected/charging alters the readings on the battery pin!
// // So we use the following...
// // - Ignoring documented conversion ratios for pin readings -> voltage conversions, we based our pin readings on physical multimeter readings
// // - Actual power readings of plugged in power supply aren't really important - just if it plugged in or not
// // - Voltage range on battery pin is different for battery only or plugged in at same time, so we use 2 voltage ranges to measure 0% -> 100% battery left
// // - As battery becomes full it becomes hard to distinguish between power only (no battery) and power + battery charging

//   // We need to know the state of the power pin to know which range to use for the battery pin measurements,
//   // as the battery pin is affected by the power pin (charging or not charging)
//   RawPowerSensorReading = analogRead(POWER_MONITOR_PIN);

//   // Power present skews the reading (puts extra voltage on the battery pin when charging, or false voltage on battery pin if no battery)
//   // so we artificially reduce the reading to estimate what actual battery is
//   // Experiments showed around 34% more voltage than actual battery voltage when charging, and around 90% of actual battery voltage when no battery connected (but powered by USB)
//   // We compensate by knocking off ~ 0.14 volts from the reading
//   // All other estimates, ranges etc. should then work based on this adjusted `pretend` battery value
//   if (RawPowerSensorReading > PWR_PRESENT_THRESHOLD)
//   {
//     RawPinReading -= ((float)(BATTERY_MAX - BATTERY_MIN) * 0.34); // Reduce reading by 34% of the range to estimate actual battery voltage);
//   }

//   RawBatteryVoltage = fmap(RawPinReading, BATTERY_MIN, BATTERY_MAX, BATTERY_MINV, BATTERY_MAXV);

//   // ALTERNATIVE
//   // Drain battery till things JUST fail
//   // Measure manually battery charge at that point - that's our base line we dont want to go under (rather than just 3.3 being the bottom)
//   // Set that as battery minimum
//   // Charge battery till won't charge any more
//   // Measure manually battery charge at that point - thats our top line we don't want to go over (rather than just 3.7 being the top)
//   // FIND A BASE LINE READING (multimeter included) FOR LOWEST NON CRASHING BATTERY READING

//   ClampedBatterySensorReading = RawPinReading;
//   // Manual clamp for easy wrapping of serial information

//   if (ClampedBatterySensorReading > BATTERY_MAX)
//   {
// #if defined(EXTRA_SERIAL_DEBUG)
//     Serial.printf("🔋 ⚠️ Battery sensor reading was above the max value! Max: %d, Reading: %d\n", BAT_MAX, ClampedBatterySensorReading);
// #endif
//     ClampedBatterySensorReading = BATTERY_MAX;
//   }
//   else if (ClampedBatterySensorReading < BATTERY_MIN)
//   {
// #if defined(EXTRA_SERIAL_DEBUG)
//     Serial.printf("🔋 ⚠️ Battery sensor reading was below the min! Min: %d, Reading: %d\n", BAT_MIN, ClampedBatterySensorReading);
// #endif
//     ClampedBatterySensorReading = BATTERY_MIN;
//   }

//   ClampedBatteryPercentage = fmap(ClampedBatterySensorReading, BATTERY_MIN, BATTERY_MAX, 0.0, 100.0);
//   ClampedVoltage = fmap(ClampedBatterySensorReading, BATTERY_MIN, BATTERY_MAX, BATTERY_MINV, BATTERY_MAXV);

//   // Work out if we are powered by battery, usb, or charging the battery
//   // Note there is no `charging` state we can actually query, so we estimate based on
//   // battery level and if we are powered by USB or not.
//   // Assumption is
//   // ...100% battery + USB power = usb powered
//   // ...other battery + USB power = charging
//   // ...else battery powered

//   // float powerVoltage = (PowerSensorReading / ADC_RESOLUTION) * ADC_REF; // * PWR_DIVIDER_RATIO;

//   if (RawPowerSensorReading > PWR_PRESENT_THRESHOLD)
//   {
//     if (ClampedBatteryPercentage == 100)
//       State = POWER_USB; // Powered by USB, but battery is full, so not charging
//     else
//       State = POWER_Charging; // Powered by USB and battery is not full, so we are charging
//   }
//   else
//     State = POWER_Battery; // Not powered by USB, so we are on battery

//   Serial.printf("🔋RawPinReading: %.2f, PowerSensorReading: %d, RawVoltage: %f, Battery Reading: %d, Battery %: %d, Approx Battery Voltage: %.2f, State: %s\n",
//                 RawPinReading,
//                 RawPowerSensorReading,
//                 RawBatteryVoltage,
//                 ClampedBatterySensorReading,
//                 ClampedBatteryPercentage,
//                 ClampedVoltage,
//                 (State == POWER_Battery) ? "Battery" : (State == POWER_USB) ? "USB"
//                                                                             : "Charging");

// #ifdef EXTRA_SERIAL_DEBUG_PLUS
//   Serial.println("Battery Sensor Limited: " + String(CurrentBatterySensorReading) + ", Battery %: " + String(CurrentBatteryPercentage) + ", Approx Battery Voltage: " + String(Voltage));
// #endif
// }

#define BatteryEmptyXPos ((SCREEN_WIDTH - 48) >> 1)
#define BatteryEmptyYPos ((SCREEN_HEIGHT - 16) >> 1)

IconRun BatteryEmptyGfx[] = {
    {.StartIcon = Icon_BigBattery_Empty1, .Count = 3, .XPos = BatteryEmptyXPos, .YPos = BatteryEmptyYPos}};

int BatteryEmptyGfx_RunCount = sizeof(BatteryEmptyGfx) / sizeof(BatteryEmptyGfx[0]);

// Pass the exitInput here so we can display a label
void Battery::DrawFullDisplay(int secondRollover, int secondFlipFlop, bool canContinue, Input continueInput, bool includeLED)
{
  // Not very optimal drawing, but we don't care, we aren't doing anything else now
  bool isCharging = (Battery::State == PowerState::USB_Battery_Charging);

  Display.clearDisplay();

  int xPos = (SCREEN_WIDTH - 48) >> 1;

  // Draw big battery (includes empty gfx)
  RenderIconRuns(BatteryEmptyGfx, BatteryEmptyGfx_RunCount);

  // We only want the empty gfx if < 5%
  if (secondFlipFlop || ClampedPercentage >= POWER_Percentage_Empty)
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
    snprintf(buffer, sizeof(buffer), "Charging... %d%%", ClampedPercentage);
  else
    snprintf(buffer, sizeof(buffer), "Battery... %d%%", ClampedPercentage);
  RRESmall.printStr(ALIGN_CENTER, 0, buffer);

  // Draw text...
  if (canContinue)
    snprintf(buffer, sizeof(buffer), "Hold %s to continue", DIGITALINPUT_BATTERY_CONTINUE_LABEL);
  else
    snprintf(buffer, sizeof(buffer), "Charge to %d%% to continue", POWER_Percentage_Empty);

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
        Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, (int)((40.0 * f * ClampedPercentage) / 100.0), 12, C_WHITE);
        Display.display();
        delay(15);
      }
    }
    else
    {
      // Just draw a percentage full battery bar, no animation
      Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, (int)((40.0 * ClampedPercentage) / 100.0), 12, C_WHITE);
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