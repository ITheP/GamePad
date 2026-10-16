#include "Structs.h"
#include "Debug.h"
#include "DeviceConfig.h"
#include "Battery.h"

void setupBattery()
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

  Serial_INFO;
  Serial.println("🔋 Setting up Battery...");

  // Battery hardware monitoring
  pinMode(POWER_MONITOR_PIN, INPUT);
  pinMode(BATTERY_MONITOR_PIN, INPUT);
  // measure 0->2.2v - theoretically voltage divider on e.g. 3.8v needs 1.9v
  // 0db is more accurate but only 1.1v in range
  analogSetPinAttenuation(BATTERY_MONITOR_PIN, ADC_6db);
  // TODO: Can we use uint32_t voltage_mv = analogReadMilliVolts(BATTERY_MONITOR_PIN); ? Probably not, voltage range isn't wide enough. May be able to use it for equivalencies.

  Battery::Init();

  // Make sure we have atleast one battery reading completed
  Battery::TakeReading();

  // Copy this into Raw_Battery so exponential smoothing isn't starting from 0
  Battery::Raw_Battery = Battery::Raw_CumulativeReadings;
}

#ifdef LIVE_BATTERY
void enforceBatteryChargeState()
{
  // In a live environment, we never continue if battery is too low.
  // It's too critical and the device will fail to operate if this happens!
  // Drop too low and battery damage can occur.
  // We spend critical time warning people.
  // Note that during development, a test board might have no battery connected to produce a measurable voltage,
  // meaning a permanent 0 battery level. In this case undefine LIVE_BATTERY to skip power checks and lots of battery stuff.

  // Scenarios when we hit this point (usually on boot)
  // Usually we expect device to operate on battery.
  // If battery is charging/on USB, we go into the special charge loop below (user is able to exit it)
  // If battery is < 5%, we go into the charge loop (user prompted to charge)
  // ...otherwise we boot as normal
  // In the charge loop, if charge moves from < 5% to > 5% then user is able to continue

  Battery::TakeReading();
  Battery::CalculateState();

  // If power is too low or we are in a charging stage, boot to charge screen
  if (POWER_Percentage_Low > Battery::ClampedPercentage || Battery::IsCharging)
  {
    // Simulate SecondRollover and SecondFlipFlop + loop forever
    int flipFlop = false;
    bool canContinue = false;
    
    // IMPORTANT
    // Recommend the battery continue button that exits the loop below ISN'T the same as
    // one of the profile buttons, else handling inputs may become somewhat conflicted

    while (1)
    {
      canContinue = (POWER_Percentage_Low < Battery::ClampedPercentage || Battery::IsCharging);

      Battery::ProcessInputs();

      // check if button held down to exit
      if (canContinue && Battery::ContinueIsPressed())
      {
        // Jump out of the while loop
        break;
      }

      Battery::ProcessFullDisplayState(true, flipFlop, canContinue, false);
      flipFlop = !flipFlop;

      // Read new battery reading
      Battery::TakeReading();
      Battery::CalculateState();
    }

    Serial.println("Continue was hit");
  }
}
#endif
