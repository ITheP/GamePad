#pragma once

#include "DeviceConfig.h"
#include "Debug.h"

// const float Vcc = 2.4;

// const float Bat_MaxVoltage = 4.2;       // Max voltage of our external battery. May physically go higher, but thats fine, this is the theoretical normal max.
// const float Bat_MaxReadVoltage = 2.4;   // Our external voltage max value after accounting for voltage divider of incoming battery level. Also means the analogRead of this never maxes out and stays within a more accurate reading range on ESP32-S3

enum class PowerState : uint8_t
{
    Battery = 0,              // Power coming from battery
    Battery_Empty = 1,        // Power really low - effectively empty
    Battery_Full = 2,         // Power coming from battery, battery is also full
    USB_Battery_Charging = 3, // Power from USB, battery charging
    USB_Battery_Full = 4,     // Power from USB, battery full
    USB = 5,                  // Power coming from external source (USB or other)
    Unknown = 6               // Battery is being charged by external source (USB or other)
};

// Array to map PowerState enum values to their string representation
const char* const PowerStateNames[] = {
    "Battery",                  // PowerState::Battery (0)
    "Battery Empty",             // PowerState::Battery_Empty (1)
    "Battery Full",             // PowerState::Battery_Full (2)
    "USB + Battery Charging",   // PowerState::USB_Battery_Charging (3)
    "USB + Battery Full",       // PowerState::USB_Battery_Full (4)
    "USB",                      // PowerState::USB (5)
    "Unknown"                   // PowerState::Unknown (6)
};

#define POWER_Percentage_Empty 0 // Percentage at which we consider battery to be empty and need to charge
#define POWER_Percentage_Low 5   // Percentage at which we consider battery to be empty and need to charge

// // Battery level's on an esp32-s3-wroom-1 dev board with DIY battery hook up
// // Battery level information https://batteryint.com/blogs/news/18650-battery-voltage#:~:text=Minimum%20Voltage%20Threshold%3A%20When%20the%20battery%20is%20depleted%2C,avoid%20damaging%20the%20battery%27s%20internal%20structure%20and%20chemistry.
// // Measured voltage mappings
// // Relative battery mappings were made against 18650 Li-ion battery
// // % levels are not truly accurate - battery degredation is not linear. But they will do for our purposes!
// // 4.8v = 4096
// // 4.7v = 4060
// // 4.6v = 3930
// // 4.5v = 3800
// // 4.4v = 3685
// // 4.3v = 3570
// // 4.2v = 3465 - 100% possible full charge level [Li-ion]
// #define BAT_MAX 3460.0
// #define BAT_MAXV 4.2
// // 4.1v = 3360 - 90%
// // 4.0v = 3265 - 80%
// // 3.9v = 3175 - 70%
// // 3.8v = 3080 - 60%
// // 3.7v = 2990 - 50% nominal charge for 18650 [Li-ion]
// // 3.6v = 2905 - 40%
// // 3.5v = 2820 - 30%
// // 3.4v = 2735 - 25%
// // 3.3v = 2650 - 20% Equivalent of Empty - charge now
// #define BAT_MIN 2650.0
// #define BAT_MINV 3.3
// // 3.2v = 2570
// // 3.1v = 2485 Device Failing to operate any more
// // Anything below this is battery dead - charge now
// // 2.5v = potential RIP point for battery

// // Battery level's on
// // Taken from Olimex ESP32-S3-DevKit-Lipo Development Board with built in battery monitoring + USB charging
// // Predicted Pin reading @ 3.7v = 2324
// // Volt Meter: 3.565 while charging (USB plugged in) - reading at pin was 1995
// // Volt Meter: 3.504 not charging - reading at pin was 1846
// // Predicted Pin reading @ 3.3v = 1345.3
// #define BAT_MAX 2324.0
// #define BAT_MAXV 3.7
// #define BAT_MIN 1345.3
// #define BAT_MINV 3.3

// We round the values slightly above to account for inaccuracies and over sensitivity on edge cases

class Battery
{
public:
    static void TakeReading();
    static void CalculateState();
    static void ProcessFullDisplayState(int secondRollover, int secondFlipFlop, bool canContinue, bool includeLED = true);
    static void ProcessInputs();
    static const char* GetPowerStateString(PowerState state);
    static const char* StateDescription();
    static void PrintToSerial();

    inline static int ContinueIsPressed()
    {
        return DigitalInput_Battery_Continue.ValueState.Value == PRESSED;
    }

    static PowerState State;
    static int CalculationsSinceLastStateChange;

    static int Raw_CumulativeReadings;
    static int Raw_CumulativeCount;
    static int Raw_Power;

    static float Raw_Battery;
    static int PreviousPercentage;
    // static int ClampedBatterySensorReading;
    static int Percentage;
    static float fPercentage;
    static int ClampedPercentage;
    static float fClampedPercentage;
    static float Voltage;               // Calculated voltage, accounting for remapping of range if also powered (connected to power changes battery reading range)
    static float ClampedVoltage;        // Clamped to theoretical 0-100% battery range, good for UI's
    static float ActualVoltage;         // Actual voltage - good for debugging
    // static float ClampedVoltage;
    // static float RawBatteryVoltage;

    static float Minimum;               // Minimum pin reading used for calculations - good for debugging
    static float Minimum_Voltage;       // Minimum voltage used for calculations - good for debugging
    static float Maximum;               // Maximum pin reading used for calculations - good for debugging
    static float Maximum_Voltage;       // Maximum voltage used for calculations - good for debugging

    static bool IsCharging;
    static bool OnBattery;
    static bool USBPower;
    static bool BatteryFull;

    static void DitheredFill(int x, int y, int w, int h, float startPct, float endPct);
    static void DitheredFillRandom(int x, int y, int w, int h, float startPct, float endPct);

private:
    static void DrawCenteredIcon(int yPos, char c);
};

inline void Battery::TakeReading()
{
    Raw_CumulativeReadings += analogRead(BATTERY_MONITOR_PIN);
    Raw_CumulativeCount++;
}