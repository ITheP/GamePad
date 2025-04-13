#include "Battery.h"

int PreviousBatteryLevel = -1;
int CurrentBatterySensorReading = 0;
int CurrentBatteryPercentage = 0;
int CumulativeBatterySensorReadings = 0;
int BatteryLevelReadingsCount = 0;

int BatteryLevel() {
  // We have had multiple readings, so average them out and update current battery level
  CurrentBatterySensorReading = CumulativeBatterySensorReadings / BatteryLevelReadingsCount;
  CumulativeBatterySensorReadings = 0;  // Ready for next round of readings
  BatteryLevelReadingsCount = 0;        // Set up to start readings again

  if (CurrentBatterySensorReading > BAT_MAX)
    CurrentBatterySensorReading = BAT_MAX;
  else if (CurrentBatterySensorReading < BAT_MIN)
    CurrentBatterySensorReading = BAT_MIN;

  CurrentBatteryPercentage = (CurrentBatterySensorReading - BAT_MIN) * 100.0 / (BAT_MAX - BAT_MIN);

#ifdef EXTRA_SERIAL_DEBUG
  float voltage = map(CurrentBatteryPercentage, 0.0, 100.0, BAT_MINV, BAT_MAXV);
  Serial.println("Battery Sensor Limited: " + String(CurrentBatterySensorReading) + ", Battery %: " + String(CurrentBatteryPercentage) + ", Approx Battery Voltage: " + String(voltage));
#endif

  return CurrentBatteryPercentage;
}