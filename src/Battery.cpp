#include "DeviceConfig.h"
#include "Battery.h"
#include "Structs.h"
#include "IconMappings.h"
#include "Screen.h"
#include "DisplayEffects.h"
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
float Battery::fPercentage = 0;
int Battery::ClampedPercentage = 0;
float Battery::fClampedPercentage = 0;
float Battery::Voltage = 0;
float Battery::ClampedVoltage = 0;
float Battery::ActualVoltage = 0;

float Battery::Minimum = 0;
float Battery::Minimum_Voltage = 0;
float Battery::Maximum = 0;
float Battery::Maximum_Voltage = 0;

bool Battery::IsCharging = false;
bool Battery::OnBattery = false;
bool Battery::USBPower = false;
bool Battery::BatteryFull = false;

int Battery::CalculationsSinceLastStateChange = 100;

#if defined(USE_EXTERNAL_LED) && defined(ExternalLED_StatusLED)
// Forward declaration - will be defined in device config
enum class LEDStrip;
#endif

// Helper function to convert PowerState enum to a readable string using an array lookup
const char *Battery::GetPowerStateString(PowerState state)
{
  // Check if the state value is within the bounds of the array
  if ((int)state >= 0 && (int)state < sizeof(PowerStateNames) / sizeof(PowerStateNames[0]))
  {
    return PowerStateNames[(int)state];
  }
  return "???"; // Default fallback if the state is out of bounds
}

const char *Battery::StateDescription()
{
  return GetPowerStateString(State);
}

void Battery::CalculateState()
{
  if (Raw_CumulativeCount == 0)
  {
    // This may get called between a previous reading and next one, so may be nothing to use yet
    return;
  }

  // Was Raw_Battery = Raw_CumulativeReadings / Raw_CumulativeCount;
  // Moved to a exponential moving average (should be smoother)
  float rawAverage = Raw_CumulativeReadings / Raw_CumulativeCount;
  Raw_Battery =  (Raw_Battery * 0.875f) + (rawAverage * 0.125f);    // Use 12.5% new / 87.5% old:
  
  // Serial.printf("culm: %d, count: %d, bat: %.2f", Raw_CumulativeReadings, Raw_CumulativeCount,Raw_Battery );
  Raw_CumulativeReadings = 0; // Ready for next round of readings
  Raw_CumulativeCount = 0;    // Set up to start readings again

  // Calculating battery state can be a bit messy. Some of it is a bit of guess work and estimates, can depend on
  // device, individual battery, charging circuit, tolerances, which way the wind is blowing, device load, etc.
  // Voltage estimates have proved to be inaccurate in theory vs measured.
  // This version is based on a pin that measures battery level, and also connected power via usb, so we can
  // estimate if we are charging or not, if we are powered by cable or by battery.
  // Note that having a power cable connected/charging alters the readings on the battery pin!
  // So we use the following...
  // - Ignoring documented conversion ratios for pin readings -> voltage conversions, we based our pin readings on physical multimeter readings
  // - Actual power readings of plugged in power supply aren't really important - just if it plugged in or not
  // - Voltage range on battery pin is different for battery only or plugged in at same time, so we use 2 voltage ranges to measure 0% -> 100% battery left
  // - As battery becomes full it becomes hard to distinguish between power only (no battery) and power + battery charging
  // - Need to handle border cases so we don't flicker between e.g. full and charging when on the edge between them
  // We need to know the state of the power pin to know which range to use for the battery pin measurements,
  // as the battery pin is affected by the power pin (charging or not charging)
  Raw_Power = analogRead(POWER_MONITOR_PIN);

  PowerState newState;
  float min;
  float min_v;
  float full;
  float full_v;
  float max;
  float max_v;
  bool batteryRange = false;

  if (Raw_Power < POWER_PRESENT)
  {
    min = BATTERY_MIN;
    min_v = BATTERY_MIN_V;
    full = BATTERY_FULL;
    full_v = BATTERY_FULL_V;
    max = BATTERY_MAX;
    max_v = BATTERY_MAX_V;

    if (Raw_Battery < full)
    {
      if (Raw_Battery < BATTERY_MIN)
        newState = PowerState::Battery_Empty;
      else
        newState = PowerState::Battery;
    }
    else
      newState = PowerState::Battery_Full;

    batteryRange = true;
  }
  else
  {
    min = CHARGING_MIN;
    min_v = CHARGING_MIN_V;
    full = CHARGING_FULL;
    full_v = CHARGING_FULL_V;
    max = CHARGING_MAX;
    max_v = CHARGING_MAX_V;

    if ((Raw_Battery >= CHARGING_FULL) && (Raw_Power > POWER_PRESENT))
      newState = PowerState::USB_Battery_Full;
    else if ((Raw_Battery >= BATTERY_CHECK) && (Raw_Power < POWER_MAX))
      newState = PowerState::USB_Battery_Charging;
    else if ((Raw_Battery < BATTERY_CHECK) && (Raw_Power == POWER_MAX))
      newState = PowerState::USB;
    else
      // TODO: Tune this properly! Looks like there is a state missing
      newState = PowerState::USB;
    // newState = PowerState::Unknown;
  }

  // For certain types of state change...
  //   - Battery <-> Battery_Full
  //   - USB_Battery_Charging <-> USB_Battery_Full
  // ...only actually change state if been in the new state for a number of calculations

  bool updateEverything = false;

  if (
      (newState == PowerState::Battery && State == PowerState::Battery_Full) ||
      (State == PowerState::Battery && newState == PowerState::Battery_Full) ||
      (newState == PowerState::USB_Battery_Charging && State == PowerState::USB_Battery_Full) ||
      (State == PowerState::USB_Battery_Charging && newState == PowerState::USB_Battery_Full))
  {
    // All these scenarios we want atleast a calculation streak of 10 before we consider the change made
    // to account for jitter between boundaries

    CalculationsSinceLastStateChange++;

    if (CalculationsSinceLastStateChange > 10)
    {
      updateEverything = true;

      CalculationsSinceLastStateChange = 0;
    }
  }
  else
  {
    // Other state changes are more definative (e.g. turning battery on/off, plugging in cable)
    updateEverything = true;
    CalculationsSinceLastStateChange = 0;
  }

  if (updateEverything)
  {
    State = newState;

    IsCharging = (newState == PowerState::USB_Battery_Charging || newState == PowerState::USB_Battery_Full);
    OnBattery = (newState == PowerState::Battery || newState == PowerState::Battery_Full);
    USBPower = (newState == PowerState::USB || IsCharging);
    BatteryFull = (newState == PowerState::Battery_Full || newState == PowerState::USB_Battery_Full);
  }

  // TODO: Not 100% sure about this
  // Only need to update below when state changes for static kinds of display
  // or if the battery might be going up or down
  if (updateEverything || IsCharging || OnBattery)
  {
    PreviousPercentage = Percentage;
    fPercentage = fmap(Raw_Battery, min, full, 0.0, 100.0); // Maybe > 100% (when overcharged)
    Percentage = (int)(fPercentage + 0.5);
    fClampedPercentage = (CLAMP(fPercentage, 0.0, 100.0)); // Clamped for nice easy UI usage
    ClampedPercentage = (int)(fClampedPercentage + 0.5);

    // Voltage, based on % against battery range, which will remap powered range to battery range if required
    Voltage = fmap(fPercentage, 0, 100, BATTERY_MIN_V, BATTERY_FULL_V); // May go > full %
    ClampedVoltage = (int)(CLAMP(fClampedPercentage, BATTERY_MIN_V, BATTERY_FULL_V));
    // Voltage, based on actual voltage of battery or powered, no remapping
    ActualVoltage = fmap(Raw_Battery, min, max, min_v, max_v);

    Minimum = min;
    Minimum_Voltage = min_v;
    Maximum = max;
    Maximum_Voltage = max_v;
  }
#ifdef EXTRA_SERIAL_DEBUG_PLUS
  PrintToSerial();
  // Serial.printf(
  //     "🔋%-22s - "
  //     "RawBat: %.2f, RawPower: %d, %%: %.2f%% -> %d%% -> %d%%, "
  //     "V: %.2fv, ClampedV: %.2fv, ActualV: %.2fv, "
  //     "IsCharging: %s, OnBattery: %s, USBPower: %s, BatteryFull: %s, "
  //     "Pin Range: %.2f->%.2f, V Range: %.2fv->%.2fv, "
  //     "Bat: [%.2f->%.2f->%.2f , %.2f->%.2f->%.2f], "
  //     "Charging: [%.2f->%.2f->%.2f , %.2f->%.2f->%.2f], "
  //     "%.2f%%=low, %.2f%%=empty\n",

  //     GetPowerStateString(newState),
  //     Raw_Battery,
  //     Raw_Power,
  //     percentage,
  //     Percentage,
  //     ClampedPercentage,
  //     Voltage,
  //     ClampedVoltage,
  //     ActualVoltage,
  //     IsCharging ? "true" : "false",
  //     OnBattery ? "true" : "false",
  //     USBPower ? "true" : "false",
  //     BatteryFull ? "true" : "false",
  //     Minimum,
  //     Maximum,
  //     Minimum_Voltage,
  //     Maximum_Voltage,
  //     (float)BATTERY_MIN,
  //     (float)BATTERY_FULL,
  //     (float)BATTERY_MAX,
  //     (float)BATTERY_MIN_V,
  //     (float)BATTERY_FULL_V,
  //     (float)BATTERY_MAX_V,
  //     (float)CHARGING_MIN,
  //     (float)CHARGING_FULL,
  //     (float)CHARGING_MAX,
  //     (float)CHARGING_MIN_V,
  //     (float)CHARGING_FULL_V,
  //     (float)CHARGING_MAX_V,
  //     (float)POWER_Percentage_Low,
  //     (float)POWER_Percentage_Empty);
#endif
}

void Battery::PrintToSerial()
{
  Serial.printf(
      "🔋%-22s - "
      "RawBat: %.2f, RawPower: %d, %%: %.2f%% -> %d%% -> %d%%, "
      "V: %.2fv, ClampedV: %.2fv, ActualV: %.2fv, "
      "IsCharging: %s, OnBattery: %s, USBPower: %s, BatteryFull: %s, "
      "Pin Range: %.2f->%.2f, V Range: %.2fv->%.2fv, "
      "Bat: [%.2f->%.2f->%.2f , %.2f->%.2f->%.2f], "
      "Charging: [%.2f->%.2f->%.2f , %.2f->%.2f->%.2f], "
      "%.2f%%=low, %.2f%%=empty\n",

      GetPowerStateString(State),
      Raw_Battery,
      Raw_Power,
      fPercentage,
      Percentage,
      ClampedPercentage,
      Voltage,
      ClampedVoltage,
      ActualVoltage,
      IsCharging ? "true" : "false",
      OnBattery ? "true" : "false",
      USBPower ? "true" : "false",
      BatteryFull ? "true" : "false",
      Minimum,
      Maximum,
      Minimum_Voltage,
      Maximum_Voltage,
      (float)BATTERY_MIN,
      (float)BATTERY_FULL,
      (float)BATTERY_MAX,
      (float)BATTERY_MIN_V,
      (float)BATTERY_FULL_V,
      (float)BATTERY_MAX_V,
      (float)CHARGING_MIN,
      (float)CHARGING_FULL,
      (float)CHARGING_MAX,
      (float)CHARGING_MIN_V,
      (float)CHARGING_FULL_V,
      (float)CHARGING_MAX_V,
      (float)POWER_Percentage_Low,
      (float)POWER_Percentage_Empty);
}

#define BatteryGfxXPos ((SCREEN_WIDTH - 48) >> 1)
#define BatteryCableGfxYPos (((SCREEN_HEIGHT - 16) >> 1) - 3 - FONT_SMALL_HEIGHT)
#define BatteryGfxYPos (BatteryCableGfxYPos + 8 + 4)

IconRun BatteryBlankGfx[] = {
    {.StartIcon = Icon_BigBattery_Blank1, .Count = 3, .XPos = BatteryGfxXPos, .YPos = BatteryGfxYPos}};

IconRun BatteryEmptyGfx[] = {
    {.StartIcon = Icon_BigBattery_Empty1, .Count = 3, .XPos = BatteryGfxXPos, .YPos = BatteryGfxYPos}};

IconRun BatteryFullGfx[] = {
    {.StartIcon = Icon_BigBattery_Full1, .Count = 3, .XPos = BatteryGfxXPos, .YPos = BatteryGfxYPos}};

IconRun BatteryFullPlusGfx[] = {
    {.StartIcon = Icon_BigBattery_FullPlus1, .Count = 3, .XPos = BatteryGfxXPos, .YPos = BatteryGfxYPos}};

IconRun BatteryOffGfx[] = {
    {.StartIcon = Icon_BigBattery_Off1, .Count = 3, .XPos = BatteryGfxXPos, .YPos = BatteryGfxYPos}};

IconRun BatteryChargingGfx[] = {
    {.StartIcon = Icon_BigBattery_Charging1, .Count = 3, .XPos = BatteryGfxXPos, .YPos = BatteryGfxYPos}};

int BatteryGfx_RunCount = sizeof(BatteryEmptyGfx) / sizeof(BatteryEmptyGfx[0]);

static char Gfx_USBWithCable[] = {Icon_Wire_Horizontal, Icon_Menu_USB, 0};

void Battery::DrawCenteredIcon(int yPos, char c)
{

  int w = RREIcons.charWidth(c);
  static int centerX = (SCREEN_WIDTH - w) / 2;

  RREIcons.drawChar(centerX, yPos, c);
}

// Pass the exitInput here so we can display a label
void Battery::ProcessFullDisplayState(int secondRollover, int secondFlipFlop, bool canContinue, bool includeLED)
{
  // Not very optimal drawing, but we don't care, we aren't doing anything else now
  Display.clearDisplay();

  int xPos = (SCREEN_WIDTH - 48) >> 1;

  // Draw big battery (includes empty gfx)
  // RenderIconRuns(BatteryEmptyGfx, BatteryEmptyGfx_RunCount);

  // // We only want the empty gfx if < 5%
  // if (secondFlipFlop || ClampedPercentage >= POWER_Percentage_Empty)
  // {
  //   // Hide middle bit of our battery warning intermittently to make it flash
  //   Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, 40, 12, C_BLACK);
  // }

  // IF WE ARE CHARGING it looks like we are (certainly on this device)
  // around 0.136 volts lower than the actual reading (could be even less, need to run out battery and check level) when we are getting that 35% reading
  // Make this a device #define for an offset?
  // ALSO if plugged in and battery is not connected we get a 90% reading
  // on the battery pin it seems

  // Slight variants on Battery::StateDescription, along with (where relevant) big gfx
  switch (Battery::State)
  {
  case PowerState::Battery:
    snprintf(buffer, sizeof(buffer),
             "Battery... %d%%", ClampedPercentage);
    break;

  case PowerState::Battery_Empty:
    snprintf(buffer, sizeof(buffer),
             "Battery Empty");
    break;

  case PowerState::Battery_Full:
    snprintf(buffer, sizeof(buffer),
             "Battery Full");
    break;

  case PowerState::USB_Battery_Charging:
    snprintf(buffer, sizeof(buffer),
             "Charging... %d%%", ClampedPercentage);
    break;

  case PowerState::USB_Battery_Full:
    snprintf(buffer, sizeof(buffer),
             "USB + Battery Full");
    break;

  case PowerState::USB:
    snprintf(buffer, sizeof(buffer),
             "USB + No Battery");
    break;

  default: // PowerState::Unknown
    snprintf(buffer, sizeof(buffer),
             "Power State ???");
    break;
  }

  RRESmall.printStr(ALIGN_CENTER, 0, buffer);

#ifdef BATTERY_HASINPUTS
  {

    if (DigitalInput_Battery_ExtraInfo.ValueState.Value == PRESSED)
    {
      snprintf(buffer, sizeof(buffer), "%d", (int)Raw_Battery);
      RRESmall.printStr(0, BatteryCableGfxYPos, buffer);
      snprintf(buffer, sizeof(buffer), "%d", (int)Raw_Power);
      RRESmall.printStr(ALIGN_RIGHT, BatteryCableGfxYPos, buffer);
    }
  }
#endif

  // Draw text...
  if (canContinue)
    snprintf(buffer, sizeof(buffer), "Press %s", DIGITALINPUT_BATTERY_CONTINUE_LABEL);
  else
    snprintf(buffer, sizeof(buffer), "Charge to %d%%+", POWER_Percentage_Low);

  RRESmall.printStr(ALIGN_CENTER, SCREEN_HEIGHT - (FONT_SMALL_HEIGHT * 2), buffer);
  RRESmall.printStr(ALIGN_CENTER, SCREEN_HEIGHT - FONT_SMALL_HEIGHT, "to continue");

  int barWidth = 41;
  float stepSize = 1.0 / barWidth;

  // Fancy animation when charging, otherwise just shows battery percentage
  // if (secondFlipFlop)
  //{
  // Will animate for enough single frames to allow for a smooth animated fill bar (or equivalent) plus an extra bit for e.g. static size rendering
  // For simplicity we re-use this across all scenario effects
  for (float f = 0.0; f < 1.25; f += stepSize)
  {
#ifdef BATTERY_HASINPUTS
    {
      Display.fillRect(0, BatteryCableGfxYPos, SCREEN_WIDTH, 8, C_BLACK);

      if (DigitalInput_Battery_ExtraInfo.ValueState.Value == PRESSED)
      {
        snprintf(buffer, sizeof(buffer), "%d", (int)Raw_Battery);
        RRESmall.printStr(0, BatteryCableGfxYPos, buffer);
        snprintf(buffer, sizeof(buffer), "%d", (int)Raw_Power);
        RRESmall.printStr(ALIGN_RIGHT, BatteryCableGfxYPos, buffer);
      }
    }
#endif

    float fPerc = f;
    if (fPerc > 1.0)
      fPerc = 1.0;

    switch (Battery::State)
    {
    case PowerState::Battery:
      snprintf(buffer, sizeof(buffer),
               "Battery... %d%%", ClampedPercentage);

      DrawCenteredIcon(BatteryCableGfxYPos, Icon_USB_Disconnected);
      break;

    case PowerState::Battery_Full:
      snprintf(buffer, sizeof(buffer),
               "Full Battery");

      DrawCenteredIcon(BatteryCableGfxYPos, Icon_USB_Disconnected);
      break;

    case PowerState::USB_Battery_Charging:
      snprintf(buffer, sizeof(buffer),
               "Charging... %d%%", ClampedPercentage);

      RREIcons.printStr(ALIGN_CENTER, BatteryCableGfxYPos, Gfx_USBWithCable);
      break;

    case PowerState::USB_Battery_Full:
      snprintf(buffer, sizeof(buffer),
               "USB + Full Battery");

      RREIcons.printStr(ALIGN_CENTER, BatteryCableGfxYPos, Gfx_USBWithCable);
      break;

    case PowerState::USB:
      snprintf(buffer, sizeof(buffer),
               "USB");

      RREIcons.printStr(ALIGN_CENTER, BatteryCableGfxYPos, Gfx_USBWithCable);
      break;

    default: // PowerState::Unknown
      snprintf(buffer, sizeof(buffer),
               "Power State ???");
      break;
    }

    // 8 is how many segments our 0->1.25 loop is split up into for a n easy on/off toggle flag
    bool flipFlop = (((int)(f / (1.25 / 8))) % 2 == 0);

    if (IsCharging)
    {
      // Scenarios...
      // USB + Charging = charging icon
      // USB + Full = full information  + cable gfx

      if (PowerState::USB_Battery_Charging == State)
      {
        // Draw percentage full battery bar, with a moving fill to indicate charging
        Display.fillRect(BatteryGfxXPos, BatteryGfxYPos, 48, 12 + 4, C_BLACK);
        RenderIconRuns(BatteryChargingGfx, BatteryGfx_RunCount, RREBatteryIcons);
        //      Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, (int)((40.0 * f * ClampedPercentage) / 100.0), 12, C_WHITE);

        DitheredFillRandom(
            BatteryGfxXPos + 2,
            BatteryGfxYPos + 2,
            (int)(((float)barWidth * fPerc * ClampedPercentage) / 100.0),
            12,
            ClampedPercentage * 0.5,
            100.0f // ClampedPercentage
        );
      }
      else if (PowerState::USB_Battery_Full == State)
      {
        // Draw full bar or fullgfx

        Display.fillRect(BatteryGfxXPos, BatteryGfxYPos, 48, 12 + 4, C_BLACK);
        // RenderIconRuns(BatteryFullGfx, BatteryGfx_RunCount);

        if (flipFlop)
        {
          if (Percentage > 100)
            RenderIconRuns(BatteryFullPlusGfx, BatteryGfx_RunCount, RREBatteryIcons);
          else
            RenderIconRuns(BatteryFullGfx, BatteryGfx_RunCount, RREBatteryIcons);
        }
        else
        {
          RenderIconRuns(BatteryBlankGfx, BatteryGfx_RunCount, RREBatteryIcons);
          DitheredFillRandom(
              BatteryGfxXPos + 2,
              BatteryGfxYPos + 2,
              barWidth,
              12,
              ClampedPercentage * 0.25,
              100.0f // ClampedPercentage
          );
        }
      }
      // Display.display();
      // delay(15);
    }
    else
    {
      // Senarios...
      // Battery only = Statically filled %
      // Battery only is full = full battery gfx
      // USB only = Cable animation

      if (PowerState::Battery == State)
      {
        // Draw percentage full battery bar
        Display.fillRect(BatteryGfxXPos, BatteryGfxYPos, 48, 12 + 4, C_BLACK);
        RenderIconRuns(BatteryBlankGfx, BatteryGfx_RunCount, RREBatteryIcons);

        DitheredFillRandom(
            BatteryGfxXPos + 2,
            BatteryGfxYPos + 2,
            (int)(((float)barWidth * ClampedPercentage) / 100.0),
            12,
            ClampedPercentage * 0.5,
            100.0f // ClampedPercentage
        );
      }
      else if (PowerState::Battery_Empty == State)
      {
        // Do nothing, USB cable already drawn
        Display.fillRect(BatteryGfxXPos, BatteryGfxYPos, 48, 12 + 4, C_BLACK);

        if (flipFlop)
        {
          RenderIconRuns(BatteryEmptyGfx, BatteryGfx_RunCount, RREBatteryIcons);
        }
        else
        {
          RenderIconRuns(BatteryBlankGfx, BatteryGfx_RunCount, RREBatteryIcons);
        }
      }
      else if (PowerState::USB == State)
      {
        Display.fillRect(BatteryGfxXPos, BatteryGfxYPos, 48, 12 + 4, C_BLACK);

        if (flipFlop)
        {
          RenderIconRuns(BatteryOffGfx, BatteryGfx_RunCount, RREBatteryIcons);
        }
        else
        {
          RenderIconRuns(BatteryBlankGfx, BatteryGfx_RunCount, RREBatteryIcons);
        }
      }
      else if (PowerState::Battery_Full == State)
      {
        // Draw percentage full battery bar
        Display.fillRect(BatteryGfxXPos, BatteryGfxYPos, 48, 12 + 4, C_BLACK);

        if (flipFlop)
        {
          if (Percentage > 100)
            RenderIconRuns(BatteryFullPlusGfx, BatteryGfx_RunCount, RREBatteryIcons);
          else
            RenderIconRuns(BatteryFullGfx, BatteryGfx_RunCount, RREBatteryIcons);
        }
        else
        {
          RenderIconRuns(BatteryBlankGfx, BatteryGfx_RunCount, RREBatteryIcons);

          DitheredFillRandom(
              BatteryGfxXPos + 2,
              BatteryGfxYPos + 2,
              barWidth,
              12,
              ClampedPercentage * 0.5,
              100.0f // ClampedPercentage
          );
        }
      }
      else if (PowerState::Unknown == State)
      {
        RREDefault.printStr(-1, BatteryGfxYPos, "Unknown Power State");
      }
      // // Just draw a percentage full battery bar, no animation

      // Display.fillRect(BatteryGfxXPos + 2, BatteryGfxYPos + 2, barWidth, 12, C_BLACK);
      // // Display.fillRect(BatteryEmptyXPos + 2, BatteryEmptyYPos + 2, (int)((40.0 * ClampedPercentage) / 100.0), 12, C_WHITE);

      // DitheredFillRandom(
      //     BatteryGfxXPos + 2,
      //     BatteryGfxYPos + 2,
      //     barWidth,
      //     12,
      //     ClampedPercentage * 0.5,
      //     100.0f // ClampedPercentage
      // );
    }
    Display.display();

    // Delay of 250ms but we want to continue more quickly if the state changes
    PowerState tmpState = State;

    // 5 * 50 = 250ms
    for (int i = 0; i < 5; i++)
    {
      // Grab several readings to average out for more accuracy
      // 10*5 = 50ms
      for (int j = 0; j < 10; j++)
      {
        Battery::TakeReading();

        ProcessInputs();
        if (ContinueIsPressed())
          return;

        delay(5);
      }

      Battery::CalculateState();
      if (tmpState != State)
      {
        // Exit everything early
        return;
      }
    }
  }
  //}

  //   // TODO: - LED-s either effects (e.g. charging led's) or all off (probably all off + effect override)
  // #if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
  //   if (includeLED)
  //   {
  //     // ToDo: If LED's were turned on at time this was instigated, might remain on. Double check to make sure these are turned off. (Less power drain then)
  //     if (secondFlipFlop)
  //       StatusLed[0] = CRGB::Black;
  //     else
  //       StatusLed[0] = CRGB::Red;

  // #if defined(USE_EXTERNAL_LED) && defined(ExternalLED_StatusLED)
  //     // Always make sure the external status LED is updated too
  //     ExternalLeds[(int)LEDStrip::Status] = StatusLed[0]; // ExternalLED_StatusLED] = StatusLed[0];
  // #endif

  //     FastLED.show();
  //   }
  // #endif
}

// Simplified check for digital inputs for when in battery boot state
// Simply set's flags if something is pressed
// It's up to battery boot code to work out how to handle.
// TODO: Code is effectively identical to the Config menu input check, so could be refactored to a single method
void Battery::ProcessInputs()
{
  uint16_t state;
  Input *input;

  // Just in case these are used for digital input virtual pins
  for (int i = 0; i < AnalogInputs_Count; i++)
  {
    input = AnalogInputs[i];

    uint16_t analogState = 0;

    if (input->Pin != NONE) {
      uint32_t sum = 0;

      for (int i = 0; i < 4; i++) {
          sum += analogRead(input->Pin);
          delayMicroseconds(60);   // 60–80 µs is enough
      }

      analogState = sum >> 2;

      // << 3 - <- slightly faster than *8 :)
      uint32_t smoothed = (analogState << 3) - analogState + input->AnalogRaw;
      analogState = smoothed >> 3;
      input->AnalogRaw = analogState;
    }

    // Set triggered state if above TriggerOnValue
    if (input->TriggerOnValue > 0 && analogState > input->TriggerOnValue)
    {

      // Serial.println("Analog Input Set: " + String(input->Label));
      input->ValueState.Value = PRESSED;
    }
    else
    {
      // Serial.println("Analog Input Cleared: " + String(input->Label));
      input->ValueState.Value = NOT_PRESSED;
    }
  }

  for (int i = 0; i < DigitalInputs_Battery_Count; i++)
  {
    input = DigitalInputs_Battery[i];

    input->ValueState.StateJustChanged = false;

    unsigned long timeCheck = micros();
    if ((timeCheck - input->ValueState.StateChangedWhen) > DEBOUNCE_DELAY)
    {
      // May not have set the in state by this point in time so - lets make sure!

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

static const uint8_t bayer8x8[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}};

// Draw a rectangle with a left→right dither gradient
void Battery::DitheredFill(int x, int y, int w, int h, float startPct, float endPct)
{
  // One random offset per frame
  uint8_t ox = esp_random() & 7;        // 0–7
  uint8_t oy = (esp_random() >> 3) & 7; // 0–7

  float pctStep = (endPct - startPct) / (float)w;

  for (int dx = 0; dx < w; dx++)
  {
    float pct = startPct + pctStep * dx;
    uint8_t threshold = (uint8_t)((pct * 64.0f) / 100.0f); // 0–63

    for (int dy = 0; dy < h; dy++)
    {
      uint8_t d = bayer8x8[(dy + oy) & 7][(dx + ox) & 7];

      if (d < threshold)
        Display.drawPixel(x + dx, y + dy, C_WHITE);
    }
  }
}

void Battery::DitheredFillRandom(int x, int y, int w, int h,
                                 float startDensityPercentage, float endDensityPercentage)
{
  float pctStep = (endDensityPercentage - startDensityPercentage) / (float)w;

  for (int dx = 0; dx < w; dx++)
  {
    float pct = startDensityPercentage + pctStep * dx;      // 0–100%
    uint8_t threshold = (uint8_t)((pct * 255.0f) / 100.0f); // 0–255

    // One random seed per column → stable vertical grain
    uint32_t seed = esp_random();

    for (int dy = 0; dy < h; dy++)
    {
      // Fast LCG pseudo-random (no expensive ops)
      seed = seed * 1664525u + 1013904223u;
      uint8_t r = seed >> 24; // top byte = random 0–255

      if (r < threshold)
        Display.drawPixel(x + dx, y + dy, C_WHITE);
    }
  }
}