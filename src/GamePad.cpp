#include <Wire.h>
// #include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RREFont.h>
#include <BleGamepad.h>
#include "Config.h"
#include "IconMappings.h"
#include "Screen.h"
#include "Icons.h"
#include "Device.h"
#include "Stats.h"
#include "RenderText.h"
#include "Benchmark.h"
#include "Battery.h"
#include "GamePad.h"

// Individual controller configuration and pin mappings come from specific controller specified in DeviceConfig.h
#include "DeviceConfig.h"

// -----------------------------------------------------
// LED stuff (include after controller definition)
#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
#include <FastLED.h>

CRGB ExternalLeds[ExternalLED_Count];
int ExternalLedsEnabled[ExternalLED_Count];

#include "LED.h"

// Task for handling FastLED updates
void UpdateExternalLEDs(void *parameter)
{

  float statusDecrease = (255.0 * EXTERNAL_LED_FADE_RATE * LED_UPDATE_RATE);
  uint8_t externalDecrease = (uint8_t)(255.0 * EXTERNAL_LED_FADE_RATE * LED_UPDATE_RATE);

  UpdateExternalLEDsLoop(statusDecrease, externalDecrease);
}

#endif

// -----------------------------------------------------
// Logo/Splashscreen

IconRun Logo[] = {
    {.StartIcon = Icon_Logo_Start1, .Count = 8, .XPos = 0, .YPos = 0},
    {.StartIcon = Icon_Logo_Start2, .Count = 8, .XPos = 0, .YPos = 16},
    {.StartIcon = Icon_Logo_Start3, .Count = 8, .XPos = 0, .YPos = 32}};
int Logo_RunCount = sizeof(Logo) / sizeof(Logo[0]);

// -----------------------------------------------------
// Gamepad

BleGamepad bleGamepad;
bool BTConnectionState;
bool PreviousBTConnectionState;

// Buffers
char buffer[64]; // More than big enough for anything we are doing here

// -----------------------------------------------------
// Battery

inline void TakeBatteryLevelReading()
{
  CumulativeBatterySensorReadings += analogRead(BATTERY_MONITOR_PIN);
  BatteryLevelReadingsCount++;
}

// -----------------------------------------------------
// Benchmarks

#ifdef INCLUDE_BENCHMARKS
Benchmark MainBenchmark("PERF.Main");
#endif

// -----------------------------------------------------
// Misc
// Serial.print(" @ " + String((uintptr_t)config, HEX));

int DebounceDelay = DEBOUNCE_DELAY;

int Frame = 0;
unsigned long PreviousMicroS = 0;

float Now;
float PreviousNow;

int PreviousSecond = -1;
int Second = 0;
int SecondRollover = false;       // Flag to easily sync operations that run once per second
int SecondFlipFlop = false;       // Flag flipping between 0 and 1 every second to allow for things like blinking icons

int PreviousSubSecond = -1;
int SubSecond = 0;
int SubSecondRollover = false;    // SubSecond flag for things like statistics sampling
int SubSecondFlipFlop = false;

char LastBatteryIcon = 0;
int LastSerialState = -1;

float PreviousFractionalSeconds = 0;
float CurrentFractionalSeconds = 0;
float ExtendedFractionalSeconds = 0;

int LEDUpdateRollover = false;    // Throttling flag for LED updates - may be far faster than e.g. 60fps to allow for effects to propagate more quickly
float NextLEDUpdatePoint = 0;

int DisplayRollover = false;      // Throttling flag for display updates
float NextDisplayUpdatePoint = 0;

void setup()
{
  // delay(10); // Give pins on start up time to settle if theres any power up glitching
  Wire.begin(I2C_SDA, I2C_SCL);

  // ResetPrintDisplayLine();  // Not currently used // Make sure any rendering of text starts top left of screen

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // if (!Display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, &Wire, OLED_RESET)) {
  if (!Display.begin(SCREEN_ADDRESS, true))
  {
    // Alternative handling - Spit out an error over serial connection if problem with display
    Serial.begin(SERIAL_SPEED);
    Serial.println("Critical failure, display failed to start. Halting!");
    while (1)
    {
      // Forever flash internal LED to let us know theres something wrong!
      digitalWrite(LED_BUILTIN, HIGH);
      delay(150);
      digitalWrite(LED_BUILTIN, LOW);
      delay(150);
    }
  }

  Display.clearDisplay();
  Display.display();

  RRE.init(RRERect, SCREEN_WIDTH, SCREEN_HEIGHT);
  RRE.setCR(0);
  RRE.setScale(1);

  // Show Logo
  SetFontCustom();
  RenderIconRuns(Logo, Logo_RunCount);
  Display.display();

  // Animate USB socket coming into screen from off left, final resting place Xpos = 16 pixels in
  for (int i = -32; i <= 0; i++)
  {
    RenderIcon(Icon_Wire_Horizontal, i, 53, 16, 9);
    RenderIcon(Icon_USB_Unknown, i + 16, 53, 16, 9);
    Display.display();
    delay(15);
  }

  Serial.println("Starting up...");
  Serial.println("Checking serial...");

  Serial.begin(SERIAL_SPEED);

  // Give the serial connection 0.5 seconds to do something - and bail if no connection during that time
  unsigned long stopWaitingTime = micros() + 500000;
  while (!Serial)
  {
    if (micros() > stopWaitingTime)
    {
      break;
    }
  }

  if (Serial)
  {
    RenderIcon((unsigned char)Icon_USB_Connected, 16, 53, 16, 16);
  }
  else
  {
    RenderIcon((unsigned char)Icon_USB_Disconnected, 16, 53, 16, 16);
    Serial.println("Serial connected");
  }

  Display.display();
  delay(SETUP_DELAY);

  Serial.println("\nHardware...");
  Serial.println("SRam: " + String(ESP.getHeapSize()) + " (" + String(ESP.getFreeHeap()) + " free)");
  // May need compile time configure to get PSRam support - https://thingpulse.com/esp32-how-to-use-psram/?form=MG0AV3
  Serial.println("PSRam: " + String(ESP.getPsramSize()) + " (" + String(ESP.getFreePsram()) + " free)");

  // Controls
  RRE.drawChar(85, 50, (unsigned char)Icon_EmptyCircle_12);
  Display.display();
  delay(SETUP_DELAY / 2);
  RRE.drawChar(87, 52, (unsigned char)Icon_FilledCircle_8);
  Display.display();

  // Digital Inputs
  Serial.println("\nButton/Digital Inputs: " + String(DigitalInputs_Count));

  Input *input;
  for (int i = 0; i < DigitalInputs_Count; i++)
  {
    input = DigitalInputs[i];

    Serial.print("... " + String(input->Label) + " -> pin " + String(input->Pin));

    pinMode(input->Pin, INPUT_PULLUP);
    input->ValueState.Value = input->DefaultValue;

#ifdef USE_EXTERNAL_LED
    ExternalLEDConfig *config = input->LEDConfig;

    if (config != nullptr)
    {
      Serial.print(", external LED " + String(config->LEDNumber));
      InitExternalLED(config, ExternalLeds);
    }
    else
      Serial.print(", No LED");
#endif

    Serial.println();
  }

  // Analog Inputs
  Serial.println("\nAnalog Inputs: " + String(AnalogInputs_Count));
  for (int i = 0; i < AnalogInputs_Count; i++)
  {
    input = AnalogInputs[i];

    Serial.print("... " + String(input->Label) + " -> pin " + String(input->Pin));

    pinMode(input->Pin, INPUT);
    input->ValueState.Value = input->DefaultValue;

#ifdef USE_EXTERNAL_LED
    ExternalLEDConfig *config = input->LEDConfig;

    if (config != nullptr)
    {
      Serial.print(", External LED " + String(config->LEDNumber));
      InitExternalLED(config, ExternalLeds);
    }
    else
      Serial.print(", No LED");
#endif

    Serial.println();
  }

  // Hat inputs
  Serial.println("\nHat Inputs: " + String(HatInputs_Count));
  HatInput *hatInput;
  for (int i = 0; i < HatInputs_Count; i++)
  {
    hatInput = HatInputs[i];

    Serial.print("... Hat " + String(hatInput->Label) + " pins");

    for (int j = 0; j < 4; j++)
    {
      Serial.print(" ");

      hatInput->IndividualStates[j] = HIGH;
      hatInput->IndividualStateChangedWhen[j] = 0;

      uint8_t pin = hatInput->Pins[j];
      if (pin == 0)
      {
        Serial.print("X");
      }
      else
      {
        Serial.print(pin);
        pinMode(pin, INPUT_PULLUP);
      }
    }

    hatInput->ValueState.Value = hatInput->DefaultValue;

    Serial.println();

#ifdef USE_EXTERNAL_LED
    Serial.print("... Hat " + String(hatInput->Label) + " LEDs");

    for (int i = 0; i < 9; i++)
    {
      Serial.print(" ");

      ExternalLEDConfig *config = hatInput->LEDConfigs[i];
      if (config != nullptr)
      {
        Serial.print(config->LEDNumber);
        InitExternalLED(config, ExternalLeds);
      }
      else
        Serial.print("X");
    }
#endif
  }

  delay(SETUP_DELAY);

// Flash a little LED icon up to show we are playing with LED's
#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
  RRE.drawChar(112, 49, (unsigned char)Icon_LEDOn);
  Display.display();

  FastLED.setBrightness(LED_BRIGHTNESS);

  // FastLED.Show() main loop is processed on separate thread to allow for running on other cores.
  // Note that at time of writing, core runs on same thread as main loop (1) as Bluetooth/Wifi etc. run on core 0
  // Gives us the option to play around a bit and choose where we want to run it in the future
  xTaskCreatePinnedToCore(
      UpdateExternalLEDs,
      "LEDUpdateTask",
      10000,
      NULL,
      1,
      &UpdateExternalLEDsTask,
      1);

  uint8_t hue = 0;

// Add and flash onboard led if enabled
#ifdef USE_ONBOARD_LED
  Serial.println("\n\nLED pins\nOnboard: pin " + String(ONBOARD_LED_PIN));
  FastLED.addLeds<NEOPIXEL, ONBOARD_LED_PIN>(StatusLed, 1);

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
  Serial.println("External: pin " + String(EXTERNAL_LED_PIN));
  FastLED.addLeds<EXTERNAL_LED_TYPE, EXTERNAL_LED_PIN, EXTERNAL_LED_COLOR_ORDER>(ExternalLeds, ExternalLED_FastLEDCount); // ExternalLED_Count);

  // Knightrider the external LED's
  hue = 0;

  // Show LED's so no matter how many, they are shown within a small time frame
  // Example of 5 leds using a delay of 30ms look nice and don't take too long (150ms)
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
  RRE.drawChar(112, 49, (unsigned char)Icon_LEDOn);
  Display.display();
  delay(SETUP_DELAY);
#else
  // No LED's being used!
  RRE.drawChar(112, 49, (unsigned char)Icon_LEDNone);
  Display.display();
  delay(SETUP_DELAY);
#endif

  // Bluetooth and other general config

  RRE.drawChar(54, 51, (unsigned char)Icon_BTLogo);
  Display.display();

  // Dedicated checks to Inputs that allow buttons held on startup to switch between variants of Bluetooth Id's
  // which should allow 1 controller to pair separately to multiple hosts/devices and allow for multiple
  // hosts to use same physical controller but as separate controllers
  int chipIdOffset = 0;
  Input *overrideInput = nullptr;
  for (int i = 0; i < DigitalInputs_Count; i++)
  {
    input = DigitalInputs[i];
    if (input->BluetoothIdOffset > 0)
    {
      if (digitalRead(input->Pin) == LOW)
      {
        chipIdOffset = input->BluetoothIdOffset;
        overrideInput = input;
        Serial.println("Bluetooth Id Override : " + String(chipIdOffset));
      }
    }
  }

  if (chipIdOffset > 0)
  {
    // Extra Bluetooth iconography to show that one of the extra bluetooth Id variants is active
    SetFontFixed();
    RRE.drawChar(65, 50, '+');
    delay(100);
    Display.display();
    snprintf(buffer, sizeof(buffer), "%d", chipIdOffset);

    // If we found a BluetoothId override input, flash it (as long as it has an ExternalLEDPrimaryColour LED and OnboardLED defined with enabled values)
    // We get the associated LED and colours from whatever control defined the override
#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
    if (overrideInput != nullptr)
    {
      LED *onboardLed = nullptr;
      LED *extLed = nullptr;

      // Check if onboard LED is defined and enabled
#ifdef USE_ONBOARD_LED
      onboardLed = &overrideInput->OnboardLED;
#endif
      // Check if input has a defined LED that is also enabled
#ifdef USE_EXTERNAL_LED
      int ledNumber = overrideInput->LEDConfig->LEDNumber;
      extLed = &overrideInput->LEDConfig->PrimaryColour;
#endif

      for (int i = 0; i < 4; i++)
      {
        // Draw bluetooth number
        RRE.printStr(73, 50, buffer);
        Display.display();

        // Flash LED's
#ifdef USE_ONBOARD_LED
        if (onboardLed != nullptr)
          StatusLed[0] = CRGB::Black;
#endif
#ifdef USE_EXTERNAL_LED
        if (extLed != nullptr)
          ExternalLeds[ledNumber] = extLed->Colour;
#endif
        FastLED.show();
        delay(150);

        // Blank Bluetooth number
        Display.fillRect(73, 50, 8, 16, C_BLACK);
        Display.display();

#ifdef USE_ONBOARD_LED
        if (onboardLed != nullptr)
          StatusLed[0] = CRGB::Black;
#endif
#ifdef USE_EXTERNAL_LED
        if (extLed != nullptr)
          ExternalLeds[ledNumber] = CRGB::Black;
#endif
        FastLED.show();
        if (i != 3)
          delay(150); // No need to delay after final switch off
      }
    }
#endif

    RRE.printStr(73, 50, buffer);
    Display.display();
    SetFontCustom();
  }

  uint64_t chipId = ESP.getEfuseMac();
  int offset = chipId % DeviceNamesCount;
  // Bluetooth controller max name length = 30 chars (+ null terminator)

  // If the below line is put before the offset = above, the name will also randomise. Currently set up for a consistent name so it doesn't get too confusing!
  chipId += chipIdOffset;

#ifdef DEVICE_NAME
  // Custom device name
  if (chipIdOffset == 0)
    snprintf(DeviceName, sizeof(DeviceName), "%s", DEVICE_NAME);
  else
  {
    snprintf(DeviceName, sizeof(DeviceName), "%s %d", DEVICE_NAME, chipIdOffset);
    Serial.println("Bluetooth device Id/name - custom value: " + String(chipIdOffset));
  }
#else
  // Random Device Name
  if (chipIdOffset == 0)
    snprintf(DeviceName, sizeof(DeviceName), "%s %d", DeviceNames[offset], offset);
  else
  {
    snprintf(DeviceName, sizeof(DeviceName), "%s %d.%d", DeviceNames[offset], offset, chipIdOffset);
    Serial.println("Bluetooth device Id/Name - system selected: " + String(chipIdOffset));
  }
#endif

  snprintf(buffer, sizeof(buffer), "Guitar %s", DeviceName);
  char type[] = "Guitar Controller";
  bleGamepad = BleGamepad(buffer, type, 100);

  Serial.println("\nBluetooth configuration...");
  Serial.println("... Name: " + String(buffer));
  Serial.println("... Type: " + String(type));
  BleGamepadConfiguration bleGamepadConfig;

  bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
  Serial.println("... Controller Type: Gamepad");

  bleGamepadConfig.setVid(VID);
  Serial.println("... VID: " + String(VID));

  bleGamepadConfig.setPid(PID);
  Serial.println("... PID: " + String(PID));

  char modelNumber[] = "Guitar 1.0";
  bleGamepadConfig.setModelNumber(modelNumber);
  Serial.println("... Model Number: " + String(modelNumber));

  char chipIdDesc[22]; // large enough to hold the uint64_t value as a string 20 chars + 1 for chipIdOffset + null terminator. Tests showed serial coming from esp32-s3 was 14 chars + the chipIdOffset
  sprintf(chipIdDesc, "%llu%d", chipId, chipIdOffset);
  bleGamepadConfig.setSerialNumber(chipIdDesc);
  Serial.println("... Serial Number: " + String(chipIdDesc));

  // TODO: Revision versions in config file
  char firmwareRevision[] = "1.0";
  bleGamepadConfig.setFirmwareRevision(firmwareRevision); // Version of this firmware
  Serial.println("... Firmware: v" + String(firmwareRevision));

  char hardwareRevision[] = "1.0";
  bleGamepadConfig.setHardwareRevision(hardwareRevision); // Version of circuit board etc.
  Serial.println("... Hardware: v" + String(hardwareRevision));

  char softwareRevision[] = "1.0";
  bleGamepadConfig.setSoftwareRevision(softwareRevision);
  Serial.println("... Software: v" + String(softwareRevision));

  bleGamepadConfig.setHidReportId(1);

  // Start, Select, Menu, Home, Back, VolumeInc, VolumeDec, b
  // TODO: Actual digital count of bluetooth devices (loop and count)
  bleGamepadConfig.setButtonCount(DigitalInputs_Count);
  Serial.println("... Buttons/Digital Input Count: " + String(DigitalInputs_Count));

  // bleGamepadConfig.setButtonCount(AnalogInputs_Count);
  Serial.println("... Analog Input Count: " + String(AnalogInputs_Count));

  bleGamepadConfig.setWhichSpecialButtons(true, true, true, true, true, true, true, true);
  bleGamepadConfig.setHatSwitchCount(HatInputs_Count);
  Serial.println("... Hat Count: " + String(HatInputs_Count));

#ifdef Enable_Slider1
  bleGamepadConfig.setIncludeSlider1(true);
  Serial.println("... Slider 1 Enabled");
#endif

#ifdef Enable_Slider2
  bleGamepadConfig.setIncludeSlider2(true);
  Serial.println("... Slider 2 Enabled");
#endif

  // Other possibilities, might want to use some time
  // bleGamepadConfig.setIncludeXAxis(false);
  // bleGamepadConfig.setIncludeYAxis(false);
  // bleGamepadConfig.setIncludeRxAxis(false);
  // bleGamepadConfig.setIncludeRyAxis(false);
  // bleGamepadConfig.setIncludeRzAxis(false);

  bleGamepadConfig.setAutoReport(false);
  bleGamepad.begin(&bleGamepadConfig);    // Note - changing bleGamepadConfig after the begin function has no effect, unless you call the begin function again

  // Battery
  pinMode(BATTERY_MONITOR_PIN, INPUT);
  // Make sure we have atleast one battery reading completed
  TakeBatteryLevelReading();

  Display.display();
  delay(SETUP_DELAY * 2);

// Screen flip control needs to be checked and set before main screen is drawn
#ifdef ENABLE_FLIP_SCREEN
  DigitalInput_FlipScreen.ValueState.Value = digitalRead(DigitalInput_FlipScreen.Pin);
  DigitalInput_FlipScreen.ValueState.StateChangedWhen = micros();
  FlipScreen(&DigitalInput_FlipScreen);
#endif

  DrawMainScreen();

  // Custom font is default used everywhere for icons - set to this by default. If using text font at all, remember to reset back to custom.
  SetFontCustom();

  Serial.println("\nSetup complete!");
}

// Draw main screen
void DrawMainScreen()
{
#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Start("DrawMainScreen");
#endif

  Display.clearDisplay();

  // Controller name
  SetFontFixed();
  ResetPrintDisplayLine();
  PrintDisplayLineCenter(DeviceName);

  SetFontCustom();
  if (ControllerGfx_RunCount > 0)
    RenderIconRuns(ControllerGfx, ControllerGfx_RunCount);

  // Whammy bar outline
  Display.drawRect(uiWhammyX, uiWhammyY, uiWhammyW, uiWhammyH, C_WHITE);

  // Bluetooth Initial
  RenderIcon(Icon_BTLogo, 54, 51, 0, 0);
  RenderIcon(Icon_EyesLeft, 65, 52, 0, 0);

  // Battery initial
  RenderIcon(Icon_Battery, 112, 53, 0, 0);

  Display.display();

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("DrawMainScreen");
#endif
}

void loop()
{
  unsigned long currentMicroS = micros(); // + 4294967295UL - 10000000;
  double FractionalSeconds = (double)(currentMicroS / 1000000.0);

  // micros() overflows after ~ 71.58 minutes (4294.97 seconds) when it reaches its maximum value of 4,294,967,295 - in testing caused some weird stuff to happen for time dependant code which suddenly hops back in time!
  // millis() overflows after around 49.7 days however doesn't have the timing resolution we want
  // So we create our own extended micros() equivalent that doesn't overflow...
  // Check if micros() is smaller then last time - i.e. overflowed back around to start again
  // if (FractionalSeconds < PreviousFractionalSeconds)    // Tried this first, but something up with FractionalSeconds sometimes being less than PreviousFractionalSeconds for certain edge case values
  if (currentMicroS < PreviousMicroS)
    // If so then account for it in our extended tracking
    ExtendedFractionalSeconds += (double)(4294967295UL / 1000000.0);

  PreviousNow = Now;
  Now = FractionalSeconds + ExtendedFractionalSeconds;
  PreviousFractionalSeconds = FractionalSeconds;
  PreviousMicroS = currentMicroS;

  Second = (int)Now % 60;

  // Second flagging - good for e.g. icons that toggle once a second or functions that don't need to run very often
  if (Second == PreviousSecond)
    SecondRollover = false;
  else
  {
    PreviousSecond = Second;
    SecondRollover = true;

    SecondFlipFlop = (Second & 1);
  }

#ifdef INCLUDE_BENCHMARKS
  int showBenchmark = SubSecondRollover;
  MainBenchmark.Start("Loop", showBenchmark);
#endif

  // SubSecond flagging - specific integer used for things like sub second sliding window calculations of UpDownCounts
  SubSecond = (int)(Now * SUB_SECOND_COUNT) % SUB_SECOND_COUNT;
  if (SubSecond == PreviousSubSecond)
    SubSecondRollover = false;
  else
  {
    PreviousSubSecond = SubSecond;
    SubSecondRollover = true;

    UpDownCount_UpdateSubSecond();

    // Opportunity for a throttled battery level reading
    // Reminder that we take multiple readings and they are later averaged out
    TakeBatteryLevelReading();
  }

  // Throttle display updates
  DisplayRollover = (Now > NextDisplayUpdatePoint);
  if (DisplayRollover)
    NextDisplayUpdatePoint += DISPLAY_UPDATE_RATE;
  

  // Throttle LED updates
  LEDUpdateRollover = (Now > NextLEDUpdatePoint);
  if (LEDUpdateRollover) NextLEDUpdatePoint += LED_UPDATE_RATE;
  
  PreviousFractionalSeconds = FractionalSeconds;

  bool sendReport = false;

  // We never continue if battery is too low - it's too critical - device will fail to operate soon! Spend that time warning people.
  // We check previous battery level here as once its too low its too low. No point in re-processing other stuff, and recharging involves turning off device.
  // Only in a live environment - test board might have no battery connected to produce a measurable voltage, meaning a permanent 0 battery level
#ifdef LIVE_BATTERY
  if (PreviousBatteryLevel == 0)
  {
    DrawBatteryEmpty(SecondRollover, SecondFlipFlop);
    return; // Sorry - no more processing! Make em go and charge things up
  }
#endif

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.Init", showBenchmark);
#endif

  // Things we don't do so often, like every second. Reduce overhead. Would it make a difference if we didn't throttle things like this? Probably not :)
  if (SecondRollover)
  {
    // Battery stuff
    int currentBatteryLevel = BatteryLevel();

    // NOTE current h/w, charging is separate to powering device so can't happen at the same time. However in the future.... :)
    bool charging = false;

    // Serial.println("Battery level: " + String(currentBatteryLevel));

    if (charging)
    {
      if (SecondFlipFlop)
        LastBatteryIcon = Icon_Battery;
      else
        LastBatteryIcon = Icon_BatteryCharging;

      RenderIcon(LastBatteryIcon, 112, 53, 12, 9); // Actual area cleared is just the area where battery rectangle or charging lightning bolt is
    }
    else if (currentBatteryLevel == 0)
    {
      // Handling of no battery, if not using full screen handling above
      if (SecondFlipFlop)
        LastBatteryIcon = Icon_Battery;
      else
        LastBatteryIcon = Icon_BatteryEmpty;

      RenderIcon(LastBatteryIcon, 112, 53, 12, 9); // Actual area cleared is just the area where battery rectangle or charging lightning bolt is

      // ...however next line will trigger full screen low battery handling
      PreviousBatteryLevel = currentBatteryLevel;
    }
    else if (PreviousBatteryLevel != currentBatteryLevel)
    {
      PreviousBatteryLevel = currentBatteryLevel;
      bleGamepad.setBatteryLevel(currentBatteryLevel);
      sendReport = true;

      // Redraw standard battery icon if required
      if (LastBatteryIcon != Icon_Battery)
      {
        LastBatteryIcon = Icon_Battery;
        RenderIcon(Icon_Battery, 112, 53, 12, 9);
      }

      // Battery level scale to 0->10 pixels
      int width = ((currentBatteryLevel / 100.0) * 10.0);
      // Render width as 0->10 pixel wide rectangle inside icon
      Display.fillRect(114, 55, 10, 5, C_BLACK);

      Display.fillRect(114, 55, width, 5, C_WHITE);
    }

    UpDownCount_SecondPassed(Second);

    SetFontFixed();

    // Right text
    Display.fillRect(88, 50, 16, 16, C_BLACK);
    PrintCenteredNumber(96, 50, UpDownMaxPerSecondEver);

    SetFontCustom();

    // Serial State
    if (LastSerialState != Serial)
    {
      // Override the score by putting up the serial state for a second

      LastSerialState = Serial;
      if (Serial)
        RenderIcon(Icon_USB_Connected, 0, 53, 16, 9);
      else
        RenderIcon(Icon_USB_Disconnected, 0, 53, 16, 9);
    }
  }

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.SecondRollover", showBenchmark);
#endif

  if (SubSecondRollover)
  {
    // Update the total count more often - looks nicer
    SetFontFixed();
    // Left text
    Display.fillRect(0, 50, 44, 16, C_BLACK);
    PrintCenteredNumber(27, 50, UpDownTotalCount);
  }

  SetFontCustom();

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.SubSecondRollover", showBenchmark);
#endif

  // Digital inputs
  uint16_t state;
  Input *input;

  for (int i = 0; i < DigitalInputs_Count; i++)
  {
    input = DigitalInputs[i];
    input->ValueState.StateJustChanged = false;

    unsigned long timeCheck = micros();
    if ((timeCheck - input->ValueState.StateChangedWhen) > DebounceDelay)
    {
      // Compare current with previous, and timing, so we can de-bounce if required
      state = digitalRead(input->Pin);

      // Serial.print(state);
      // Process when state has changed
      if (state != input->ValueState.Value)
      {
        // PRESSED!
        input->ValueState.Value = state;
        input->ValueState.PreviousValue = !state;
        input->ValueState.StateChangedWhen = timeCheck;
        input->ValueState.StateJustChanged = true;
        input->ValueState.StateJustChangedLED = true;

        // We have to check input states not just for Bluetooth inputs, as buttons might control device functionality
        if (state == LOW)
        {
          if (input->BluetoothInput != 0)
          {

            // bleGamepad.press(input->BluetoothInput);
            (bleGamepad.*(input->BluetoothPressOperation))(input->BluetoothInput);

            sendReport = true;
          }
        }
        else
        {
          // RELEASED!!
          if (input->BluetoothInput != 0)
          {
            // bleGamepad.release(input->BluetoothInput);
            (bleGamepad.*(input->BluetoothReleaseOperation))(input->BluetoothInput);

            sendReport = true;
          }
        }

        input->RenderOperation(input);

        // Any extra special custom to specific controller code
        if (input->CustomOperation != NONE)
          input->CustomOperation(input);
      }
    }
  }

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.DigitalInputs", showBenchmark);
#endif

  // Analog Inputs
  for (int i = 0; i < AnalogInputs_Count; i++)
  {
    input = AnalogInputs[i];
    state = analogRead(input->Pin);

    // We only register a change if it is above a certain level of change (kind of like smoothing it but without smoothing it)
    float threshold = .03 * 4095; // 3% change required
    int previousValue = input->ValueState.Value;
    if (state < (previousValue - threshold) || state > (previousValue + threshold))
    {
      if (state != input->ValueState.Value)
      {
        input->ValueState.Value = state;
        input->ValueState.PreviousValue = previousValue;
        input->ValueState.StateChangedWhen = micros(); // ToDo: More accurate setting here, as there have been delays

        int16_t rangedState = map(state, 0, 4095, 0, 32737); // Scale range to match Bluetooth range
        (bleGamepad.*(input->BluetoothSetOperation))(rangedState);

        input->RenderOperation(input);

        // Any extra special custom to specific controller code
        if (input->CustomOperation != NONE)
          input->CustomOperation(input);

        sendReport = true;
      }
    }

    // Analog effects are constantly calculated, shifted to other core for performance
  }

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.AnalogInputs", showBenchmark);
#endif

  // Hat inputs

  // bool hatChanged = false;
  int hatPreviousState;
  int hatCurrentState;
  int subState;
  // Hat's assume all pins are read. Will need to adjust code if this is not the case
  HatInput *hatInput;
  unsigned long timeCheck;

  for (int i = 0; i < HatInputs_Count; i++)
  {
    hatInput = HatInputs[i];
    hatInput->ValueState.StateJustChanged = false;

    hatPreviousState = hatInput->ValueState.Value;

    // Step 1, read in current states
    for (int j = 0; j < 4; j++)
    {
      // Not forgetting to account for debouncing

      // timeCheck = micros();

      if ((micros() - hatInput->IndividualStateChangedWhen[j]) > DebounceDelay)
      {
        // Might not be using all pins
        uint8_t pin = hatInput->Pins[j];
        if (pin != 0)
        {
          int individualState = digitalRead(pin);
          if (individualState != hatInput->IndividualStates[j])
          {
            hatInput->IndividualStates[j] = individualState;
            // timeCheck = micros();
            hatInput->IndividualStateChangedWhen[j] = micros();
          }
        }
      }
      // else state remains same as last time
    }

    // Step 2, copy first pin to extra buffer pin in states (faster/easier calculations, no wrapping needed)
    hatInput->IndividualStates[4] = hatInput->IndividualStates[0];

    hatCurrentState = HAT_CENTERED; // initially nothing
    subState = 1;                   // First possible value with something there (1 = Up. 0 = neutral/nothing position)

    // ToDo: FIX THE BELOW, NOT WORKING!

    // Step 3, check values accounting for 2 press diagonals. We go backwards so other states are checked before the 0 button, which lets us check the 3+0 button combination extra buffer thingy
    for (int j = 3; j >= 0; j--)
    {
      if (hatInput->IndividualStates[j] == LOW) // Pressed
      {
        hatCurrentState = subState;

        // Check the diagonal by checking next pin (i.e. both pins pressed)
        if (hatInput->IndividualStates[++j] == LOW)
          hatCurrentState++; // also pressed, so make diagonal

        break; // Bail from loop - no point in checking for further pressed keys, should be physically impossible on a hat (or be irrelevant)
      }

      subState += 2; // Hat values go 0, 2, 4, 6 for Up, Right, Down, Left, diagonals go between these
    }

    // Final check to see if things have changed since last time
    // Serial.print("Hat current state: " + String(hatPreviousState) + "" - New State: " + String(hatCurrentState));

    if (hatCurrentState != hatPreviousState)
    {
      hatInput->ValueState.Value = hatCurrentState;
      hatInput->ValueState.PreviousValue = hatPreviousState;
      hatInput->ValueState.StateChangedWhen = micros(); // TODO make this when hat last read took place though realistically bog all time will have passed
      hatInput->ValueState.StateJustChanged = true;
      hatInput->ValueState.StateJustChangedLED = true;

      // General hat array used with bluetooth library
      HatValues[hatInput->BluetoothHat] = hatCurrentState;

      // hatChanged = true;

      hatInput->RenderOperation(hatInput);

      if (hatInput->ExtraOperation[hatCurrentState] != NONE)
        hatInput->ExtraOperation[hatCurrentState](hatInput);

      // Any extra special custom to specific controller code
      if (hatInput->CustomOperation != NONE)
        hatInput->CustomOperation(hatInput);
    }
  }

  if (hatInput->ValueState.StateJustChanged)
  { // hatChanged) {
#ifdef EXTRA_SERIAL_DEBUG
    Serial.print("Hat Change: ");
    Serial.println(HatValues[0]);
#endif

    bleGamepad.setHats(HatValues[0], HatValues[1], HatValues[2], HatValues[3]);
    sendReport = true;
  }

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.Hats", showBenchmark);
#endif

  // Analog reads all vals = int16_t
  //
  // leftVrxJoystickLecture = analogRead(LEFT_VRX_JOYSTICK);
  // leftVryJoystickLecture = analogRead(LEFT_VRY_JOYSTICK);
  // rightVrxJoystickLecture = analogRead(RIGHT_VRX_JOYSTICK);
  // rightVryJoystickLecture = analogRead(RIGHT_VRY_JOYSTICK);

  // //Compute joysticks value
  // leftVrxJoystickValue = map(leftVrxJoystickLecture, 4095, 0, 0, 32737);
  // leftVryJoystickValue = map(leftVryJoystickLecture, 0, 4095, 0, 32737);
  // rightVrxJoystickValue = map(rightVrxJoystickLecture, 4095, 0, 0, 32737);
  // rightVryJoystickValue = map(rightVryJoystickLecture, 0, 4095, 0, 32737);

  // Bluetooth
  BTConnectionState = bleGamepad.isConnected();

  if (BTConnectionState == true)
  {
    if (sendReport)
    {
      bleGamepad.sendReport();

#ifdef EXTRA_SERIAL_DEBUG
      Serial.println(String(Frame) + ": BT Report Sent");
#endif
    }

    // Check if BT connection state changed at all
    if (PreviousBTConnectionState != BTConnectionState)
    {
      // Draw the connected icon
      RenderIcon(Icon_BTOK, 65, 52, 16, 12);
      PreviousBTConnectionState = BTConnectionState;

#ifdef USE_EXTERNAL_LED
      // Remove any BT LED status
      ExternalLedsEnabled[ExternalLED_StatusLED] = false;
#endif
    }

    // #if defined(USE_ONBOARD_LED) && !defined(USE_ONBOARD_LED_STATUS_ONLY)
    //     if (AnalogInputs_Whammy.Value > 2048) {
    //       StatusLed[0] = CRGB(StatusLED_R * 0.25, StatusLED_G * 0.25, StatusLED_B * 0.25);
    //     } else {
    //       StatusLed[0] = CRGB(StatusLED_R, StatusLED_G, StatusLED_B);
    //     }
    // #endif
  }
  else
  {
    // No Bluetooth - always be updating the searching icons
    if (SecondRollover)
      RenderIcon(SecondFlipFlop ? Icon_EyesLeft : Icon_EyesRight, 65, 52, 16, 12); // Blanking area big enough to cover the `OK` if required

    // Hunting for Bluetooth flashing blue status
    if (SecondFlipFlop)
    {
      StatusLED_R = 0;
      StatusLED_G = 0;
      StatusLED_B = 255;
    }

#ifdef USE_EXTERNAL_LED
    // Colour set later when StatusLED copied into ExternalLEDs array, just need to remember to enable it
    if (SecondFlipFlop)
    {
      ExternalLedsEnabled[ExternalLED_StatusLED] = true;
    }
    else
    {
      ExternalLedsEnabled[ExternalLED_StatusLED] = false;
    }
#endif
  }

  PreviousBTConnectionState = BTConnectionState;

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.Bluetooth", showBenchmark);
#endif

#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
  StatusLed[0] = CRGB((uint8_t)StatusLED_R, (uint8_t)StatusLED_G, (uint8_t)StatusLED_B);
#endif

#if defined(USE_EXTERNAL_LED) && defined(ExternalLED_StatusLED)
  // Always make sure the external status LED is updated too
  ExternalLeds[ExternalLED_StatusLED] = StatusLed[0];
#endif

#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
  // Throttle updates, no point in updating too often
  if (LEDUpdateRollover)
    UpdateLEDs = true;
#endif

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.LEDStuff", showBenchmark);
#endif

#ifdef SHOW_FPS
  // FPS display
  if (SecondRollover)
  {
    SetFontFixed();
    Display.fillRect(0, 0, 40, 14, C_BLACK);
    snprintf(buffer, sizeof(buffer), "%d", Frame);
    RRE.printStr(0, 0, buffer);
    Frame = 0;
    SetFontCustom();

#ifdef INCLUDE_BENCHMARKS
    MainBenchmark.Snapshot("Loop.FPS", showBenchmark);
#endif
  }
#endif

#ifdef WHITE_SCREEN
  // Show a completely white screen - handy for physical alignment when fitting screen panel in device
  Display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C_WHITE);
#endif

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.PreDisplay", showBenchmark);
#endif

  // And finally update the display with all the lovely changes above - throttled as quite high overhead
  if (DisplayRollover)
    Display.display();

  Frame++;

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.Display", showBenchmark);
#endif
}
