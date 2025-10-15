#pragma once

#include "DeviceConfig.h"

const float Vcc = 2.4;

const float Bat_MaxVoltage = 4.2;      // Max voltage of our external battery. May physically go higher, but thats fine, this is the theoretical normal max.
const float Bat_MaxReadVoltage = 2.4;  // Our external voltage max value after accounting for voltage divider of incoming battery level. Also means the analogRead of this never maxes out and stays within a more accurate reading range on ESP32-S3

// Battery level information https://batteryint.com/blogs/news/18650-battery-voltage#:~:text=Minimum%20Voltage%20Threshold%3A%20When%20the%20battery%20is%20depleted%2C,avoid%20damaging%20the%20battery%27s%20internal%20structure%20and%20chemistry.
// Measured voltage mappings
// Relative battery mappings were made against 18650 Li-ion battery
// % levels are not truly accurate - battery degredation is not linear. But they will do for our purposes!
// 4.8v = 4096
// 4.7v = 4060
// 4.6v = 3930
// 4.5v = 3800
// 4.4v = 3685
// 4.3v = 3570
// 4.2v = 3465 - 100% possible full charge level [Li-ion]
#define BAT_MAX 3460.0
#define BAT_MAXV 4.2
// 4.1v = 3360 - 90%
// 4.0v = 3265 - 80%
// 3.9v = 3175 - 70%
// 3.8v = 3080 - 60%
// 3.7v = 2990 - 50% nominal charge for 18650 [Li-ion]
// 3.6v = 2905 - 40%
// 3.5v = 2820 - 30%
// 3.4v = 2735 - 25%
// 3.3v = 2650 - 20% Equivalent of Empty - charge now
#define BAT_MIN 2650.0
#define BAT_MINV 3.3
// 3.2v = 2570
// 3.1v = 2485 Device Failing to operate any more
// Anything below this is battery dead - charge now
// 2.5v = potential RIP point for battery

// We round the values slightly above to account for inaccuracies and over sensitivity on edge cases

class Battery {
public:
    static void TakeReading();
    static int GetLevel();
    static void DrawEmpty(int secondRollover, int SecondFlipFlop);

    static int PreviousBatteryLevel;
    static int CurrentBatterySensorReading;
    static int CurrentBatteryPercentage;
    static int CumulativeBatterySensorReadings;
    static int BatteryLevelReadingsCount;
    static float Voltage;
};

inline void Battery::TakeReading() {
    CumulativeBatterySensorReadings += analogRead(BATTERY_MONITOR_PIN);
    BatteryLevelReadingsCount++;
}