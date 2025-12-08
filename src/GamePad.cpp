#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RREFont.h>
#include <BleGamepad.h>
#include <FS.h>
#include <LittleFS.h>
// #include <esp_private/panic_internal.h>
#include "Config.h"
#include "IconMappings.h"
#include "Screen.h"
#include "Icons.h"
#include "Stats.h"
#include "RenderText.h"
#include "RenderInput.h"
#include "Benchmark.h"
#include "Battery.h"
#include "GamePad.h"
#include "Network.h"
#include "Secrets.h"
#include "Screen.h"
#include "UI.h"
#include "Utils.h"
#include "Menus.h"
#include "Debug.h"
#include "Web.h"

#define DebugMarks

// #include "soc/soc.h"
// #include "soc/rtc_cntl_reg.h"

// Individual controller configuration and pin mappings come from specific controller specified in DeviceConfig.h
#include "DeviceConfig.h"

char DeviceName[20];
char FullDeviceName[32];
char SerialNumber[22]; // SerialNumber = large enough to hold the uint64_t value as a string 20 chars + 1 for ESPChipIdOffset + null terminator. Tests showed serial coming from esp32-s3 was 14 chars + the ESPChipIdOffset
uint64_t ESPChipId;
int ESPChipIdOffset;

void (*LoopOperation)(void);

// When configured for use, set of random names for controller. Actual name is picked using ESP32-S3's unique identity as a key to the name - means we can flash to multiple
// devices and they should all get decently unique names.
// Max 17 chars for a name (final name will be longer after adding automated bits to the end of the name for bluetooth variants)
// Anything longer, name wont fit on display we are using!
const char *DeviceNames[] = {"Ellen", "Holly", "Indy", "Smudge", "Peanut", "Claire", "Eleanor", "Evilyn", "Andy", "Toya", "Verity", "Laura", "Eagle",
                             "Kevin", "Sophie", "Carla", "Hannah", "Becky", "Trent", "Jean", "Sebastian", "Phil", "Colin", "Berry", "Bazil", "Anne",
                             "Bella", "Vivian", "Bunny", "Thomas", "Giles", "David", "John", "Penny", "Beverly", "Dannie", "Ginny", "Samantha", "Sam",
                             "Lisa", "Charlie", "Albert", "Shoe", "Stevie"};
int DeviceNamesCount = sizeof(DeviceNames) / sizeof(DeviceNames[0]);

// -----------------------------------------------------
// LED stuff (include after controller definition)
#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
#include <FastLED.h>

CRGB ExternalLeds[ExternalLED_Count];
int ExternalLedsEnabled[ExternalLED_Count];

#include "LED.h"
#include <Profiles.h>
#include <MenuFunctions.h>

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

// -----------------------------------------------------
// Buffers

char buffer[64]; // General purpose buffer for formatting text into

// -----------------------------------------------------
// Benchmarks

#ifdef INCLUDE_BENCHMARKS
Benchmark MainBenchmark("PERF.Main");
#endif

// -----------------------------------------------------
// Misc
// Serial.print(" @ " + String((uintptr_t)config, HEX));

int Frame = 0;
int FPS = 0;
unsigned long PreviousMicroS = 0;

float Now;
float PreviousNow;
float LastTimeAnyButtonPressed = 0;

int PreviousSecond = -1;
int Second = 0;
int SecondRollover = false; // Flag to easily sync operations that run once per second
int SecondFlipFlop = false; // Flag flipping between 0 and 1 every second to allow for things like blinking icons

int PreviousSubSecond = -1;
int SubSecond = 0;
int SubSecondRollover = false; // SubSecond flag for things like statistics sampling
int SubSecondFlipFlop = false;

char LastBatteryIcon = 0;
int LastSerialState = -1;

float FractionalSeconds = 0;
float PreviousFractionalSeconds = 0;
float CurrentFractionalSeconds = 0;
float ExtendedFractionalSeconds = 0;

int LEDUpdateRollover = false; // Throttling flag for LED updates - may be far faster than e.g. 60fps to allow for effects to propagate more quickly
float NextLEDUpdatePoint = 0;

int DisplayRollover = false; // Throttling flag for display updates
float NextDisplayUpdatePoint = 0;

int ControllerIdle = false;
int ControllerIdleJustUnset = false;

void setupShowBattery()
{
  Battery::TakeReading();
  int currentBatteryLevel = Battery::GetLevel();

  SetFontFixed();
  Display.fillRect(HALF_SCREEN_WIDTH, SCREEN_HEIGHT - RREHeight_fixed_8x16, HALF_SCREEN_WIDTH, RREHeight_fixed_8x16, C_BLACK);
  snprintf(buffer, sizeof(buffer), "%d%% %.1fv",
           Battery::CurrentBatteryPercentage,
           Battery::Voltage);

  RRE.printStr(ALIGN_RIGHT, SCREEN_HEIGHT - RREHeight_fixed_8x16, buffer);
  SetFontCustom();
}

void setupDisplay()
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // if (!Display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, &Wire, OLED_RESET)) {
  if (!Display.begin(SCREEN_ADDRESS, true))
  {
    // Alternative handling - Spit out an error over serial connection if problem with display
    Serial.begin(SERIAL_SPEED);
    delay(200);
    Serial.println("Critical failure, display failed to start. Halting!");
    Debug::WarningFlashes(LittleFSFailedToMount);
  }

  // Display.invertDisplay(true);

  Display.clearDisplay();
  Display.display();
}

void setupRRE()
{
  RRE.init(RRERect, SCREEN_WIDTH, SCREEN_HEIGHT);
  RRE.setCR(0);
  RRE.setScale(1);
  SetFontFixed();
  SetFontLineHeightFixed();
}

constexpr int LOGO_WIDTH = 128;
constexpr int LOGO_HEIGHT = 64;
constexpr int MAX_GLEAM_PIXELS = 128; // conservative upper bound

struct Pixel
{
  uint8_t x;
  uint8_t y;
};

Pixel gleamPixels[MAX_GLEAM_PIXELS];
int gleamCount = 0;

void RenderGlint(int frame)
{
  // Step 1: Reset logo from any previous frame
  Display.clearDisplay();
  RenderIconRuns(Logo, Logo_RunCount);

  // Step 2: Scan diagonal line
  gleamCount = 0;
  int x = -LOGO_HEIGHT + frame;

  for (int y = LOGO_HEIGHT; y > 0; --y)
  {
    if (x >= 0 && x < LOGO_WIDTH)
    {
      if (Display.getPixel(x, y))
        gleamPixels[gleamCount++] = {(uint8_t)x, (uint8_t)y};

      if (gleamCount >= MAX_GLEAM_PIXELS)
        break;
    }
    x++;
  }

  // Step 3: Glow pass
  for (int i = 0; i < gleamCount; ++i)
  {
    uint8_t x = gleamPixels[i].x;
    uint8_t y = gleamPixels[i].y;

    Display.drawFastHLine(x - 1, y - 2, 3, C_WHITE);
    Display.drawFastHLine(x - 2, y - 1, 5, C_WHITE);
    Display.drawFastHLine(x - 3, y, 7, C_WHITE);
    Display.drawFastHLine(x - 2, y + 1, 5, C_WHITE);
    Display.drawFastHLine(x - 1, y + 2, 3, C_WHITE);
  }

  Display.display();
}

void setupRenderLogo()
{
  SetFontCustom();
  RenderIconRuns(Logo, Logo_RunCount);
  Display.display();

  // Fancy arse gleam effect on start up - just to look cool
  for (int frame = 0; frame < LOGO_WIDTH + LOGO_HEIGHT + 3; frame += 3)
    RenderGlint(frame);
}

void setupBattery()
{
  // Battery
  pinMode(BATTERY_MONITOR_PIN, INPUT);
  // Make sure we have atleast one battery reading completed
  Battery::TakeReading();

  // setupShowBattery();
}

void setupUSB()
{
  // Animate USB socket coming into screen from off left, final resting place Xpos = 16 pixels in
  for (int i = -32; i <= 0; i++)
  {
    RenderIcon(Icon_Wire_Horizontal, i, uiUSB_yPos, 16, 9);
    RenderIcon(Icon_USB_Unknown, i + 16, uiUSB_yPos, 16, 9);
    Display.display();
    delay(10);
  }

  Serial.begin(SERIAL_SPEED);

  // Show battery stuff here early on, before waiting around for Serial visuals to render
  setupShowBattery();

  // Give the serial connection 0.5 seconds to do something - and bail if no connection during that time
  unsigned long stopWaitingTime = micros() + 500000;
  while (!Serial)
  {
    if (micros() > stopWaitingTime)
    {
      break;
    }
  }

  unsigned char usbState;
  if (Serial)
  {
    Serial.println("Serial connected");
    usbState = Icon_USB_Connected;
  }
  else
  {
    Serial.println("Serial disconnected");
    usbState = Icon_USB_Disconnected;
  }

  RenderIcon(usbState, 16, uiUSB_yPos, 16, 16);
  delay(SETUP_DELAY);

  // Reverse back a bit for final USB position
  for (int i = 0; i > -17; i--)
  {
    RenderIcon(Icon_Wire_Horizontal, i, uiUSB_yPos, 16, 9);
    RenderIcon(usbState, i + 16, uiUSB_yPos, 17, 10);
    Display.display();
    delay(15);
  }
}

#ifdef WIFI
#ifdef WEBSERVER
void setupWiFi()
{
  Serial.println("Initialising WiFi event monitoring");

  esp_netif_init(); // Need to do this now else the web service throws a wobbler

  WiFi.onEvent([](WiFiEvent_t event)
               {
  if (event == SYSTEM_EVENT_STA_GOT_IP) {
    Serial.println("✅ WiFi connected, starting web server");
#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("✅ WiFi connected, starting web server");
#endif
    Web::StartServer();
  } else if
  (event == SYSTEM_EVENT_STA_DISCONNECTED) {
    Serial.println("⚠️ WiFi disconnected, stopping web server");
#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("⚠️ WiFi disconnected, stopping web server");
#endif
    Web::StopServer();
  } });
}
#endif
#endif

#ifdef WEBSERVER
void setupWebServer()
{
  // WiFi actually turns server on and off

  // if (WiFi.status() != WL_CONNECTED)
  //{
  //   Serial.print("Web server not set up, WiFi is not yet connected");
  //   return;
  // }

  Serial.println("Configuring HTTP Web Server");
  Web::SetUpRoutes();

  // Full BT mode
  // esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
  //  Low energy BT mode
  // esp_bt_controller_enable(ESP_BT_MODE_BLE);
  //  Both BT simultaneously
  // esp_bt_controller_enable(ESP_BT_MODE_BTDM);

  // esp_event_loop_create_default();
  // esp_netif_create_default_wifi_sta();
  // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  // esp_wifi_init(&cfg);

  // Start server
  // WebServer.begin();
  Serial.println("Web Site source files:");
  Web::ListDir("/");
}
#endif

void setupDigitalInputs()
{
  // Digital Inputs
  Serial.println("\nButton/Digital Inputs: " + String(DigitalInputs_Count));

  Input *input;
  for (int i = 0; i < DigitalInputs_Count; i++)
  {
    input = DigitalInputs[i];

    Serial.print("... " + String(input->Label) + " -> pin " + String(input->Pin));

    pinMode(input->Pin, INPUT_PULLUP);
    input->ValueState.Value = input->DefaultValue;

    // Make sure any long press inputs (child inputs) have their parent set to help identify they are a child
    // Needed to make sure child inputs are skipped in certain circumstances - they are processed via their parent
    // LED's etc. ignore this and process all inputs always - never know what might need to be shown!
    // It is somewhat up to the designer/specifications to work out how to make sure nothing
    // conflicts if things like inputs use the same LED's as each other and what effects are in place
    if (input->LongPressChildInput != nullptr)
      input->LongPressChildInput->LongPressParentInput = input;

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
}

void setupAnalogInputs()
{
  // Analog Inputs
  Serial.println("\nAnalog Inputs: " + String(AnalogInputs_Count));

  Input *input;
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
}

void setupHatInputs()
{
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

    Serial.println();
#endif
  }
}

#ifdef USE_EXTERNAL_LED
void setupInitExternalLEDs()
{
  // Misc LEDs
  Serial.println("Misc LED Effects: " + String(MiscLEDEffects_Count));
  for (int i = 0; i < MiscLEDEffects_Count; i++)
  {
    ExternalLEDConfig *config = MiscLEDEffects[i];

    if (config != nullptr)
      InitExternalLED(config, ExternalLeds);
  }

  // Idle LEDs
  Serial.println("Idle LED Effects: " + String(IdleLEDEffects_Count));
  for (int i = 0; i < IdleLEDEffects_Count; i++)
  {
    ExternalLEDConfig *config = IdleLEDEffects[i];

    if (config != nullptr)
      InitExternalLED(config, ExternalLeds);
  }
}
#endif

void setupLEDs()
{
  SetFontCustom();
#ifdef USE_EXTERNAL_LED
  void setupInitExternalLEDs();
#endif

  // Flash a little LED icon up to show we are playing with LED's
#if defined(USE_ONBOARD_LED) || defined(USE_EXTERNAL_LED)
  RRE.drawChar(112, 49, (unsigned char)Icon_LEDOn);
  Display.display();

  FastLED.setBrightness(LED_BRIGHTNESS);

  // FastLED.Show() main loop is processed on separate thread to allow for running on other cores.
  // Note that at time of writing, thread runs on same core as main loop (1) as Bluetooth/WiFi etc. run on core 0
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
}

void setupBluetooth()
{
  // Bluetooth and other general config
  SetFontCustom();
  RRE.drawChar(uiBT_xPos, uiBT_yPos, (unsigned char)Icon_BTLogo);
  Display.display();

  // Dedicated checks to Inputs that allow buttons held on startup to switch between variants of Bluetooth Id's
  // which should allow 1 controller to pair separately to multiple hosts/devices and allow for multiple
  // hosts to use same physical controller but as separate controllers
  ESPChipIdOffset = 0;
  Input *overrideInput = nullptr;
  Input *input;
  for (int i = 0; i < DigitalInputs_Count; i++)
  {
    input = DigitalInputs[i];
    if (input->BluetoothIdOffset > 0)
    {
      if (digitalRead(input->Pin) == LOW)
      {
        ESPChipIdOffset = input->BluetoothIdOffset;
        overrideInput = input;
        Serial.println("Bluetooth Id Override : " + String(ESPChipIdOffset));
      }
    }
  }

  if (ESPChipIdOffset > 0)
  {
    // Extra Bluetooth iconography to show that one of the extra bluetooth Id variants is active
    SetFontFixed();
    RRE.drawChar(uiBTStatus_xPos, 50, '+');
    delay(100);
    Display.display();
    snprintf(buffer, sizeof(buffer), "%d", ESPChipIdOffset);

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
      int ledNumber = -1;
      if (overrideInput->LEDConfig != NULL)
      {
        ledNumber = overrideInput->LEDConfig->LEDNumber;
        extLed = &overrideInput->LEDConfig->PrimaryColour;
      }
#endif

      for (int i = 0; i < 4; i++)
      {
        // Draw bluetooth number
        RRE.printStr(uiBTStatus_xPos + 8, 50, buffer);
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
        Display.fillRect(uiBTStatus_xPos + 8, 50, 8, 16, C_BLACK);
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

    RRE.printStr(uiBTStatus_xPos + 8, 50, buffer);
    Display.display();
    SetFontCustom();
  }
}

void setupDeviceIdentifiers()
{
  ESPChipId = ESP.getEfuseMac();
  int offset = ESPChipId % DeviceNamesCount;
  // Bluetooth controller max name length = 30 chars (+ null terminator)

  // If the below line is put before the offset = above, the name will also randomise. Currently set up for a consistent name so it doesn't get too confusing!
  ESPChipId += ESPChipIdOffset;

#ifdef DEVICE_NAME
  // Custom device name
  if (ESPChipIdOffset == 0)
    snprintf(DeviceName, sizeof(DeviceName), "%s", DEVICE_NAME);
  else
  {
    snprintf(DeviceName, sizeof(DeviceName), "%s %d", DEVICE_NAME, ESPChipIdOffset);
    Serial.println("Bluetooth device Id/name - custom value: " + String(ESPChipIdOffset));
  }
#else
  // Random Device Name
  if (ESPChipIdOffset == 0)
    snprintf(DeviceName, sizeof(DeviceName), "%s %d", DeviceNames[offset], offset);
  else
  {
    snprintf(DeviceName, sizeof(DeviceName), "%s %d.%d", DeviceNames[offset], offset, ESPChipIdOffset);
    Serial.println("Bluetooth device Id/Name - system selected: " + String(ESPChipIdOffset));
  }

  snprintf(FullDeviceName, sizeof(FullDeviceName), "%s %s", ControllerDeviceNameType, DeviceName);
#endif

  // Init serial number
  sprintf(SerialNumber, "%llu%d", ESPChipId, ESPChipIdOffset);
}

void setupController()
{
  // snprintf(buffer, sizeof(buffer), "Guitar %s", DeviceName);
  bleGamepad = BleGamepad(FullDeviceName, ControllerType, 100);

  Serial.println("\nBluetooth configuration...");
  Serial.println("... Name: " + String(buffer));
  Serial.println("... Type: " + String(ControllerType));
  BleGamepadConfiguration bleGamepadConfig;

  bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
  Serial.println("... Controller Type: Gamepad");

  bleGamepadConfig.setVid(VID);
  Serial.println("... VID: " + String(VID));

  bleGamepadConfig.setPid(PID);
  Serial.println("... PID: " + String(PID));

  bleGamepadConfig.setModelNumber(ModelNumber);
  Serial.println("... Model Number: " + String(ModelNumber));

  bleGamepadConfig.setSerialNumber(SerialNumber);
  Serial.println("... Serial Number: " + String(SerialNumber));

  Serial.println("... Core build: " + String(getBuildVersion()));

  // TODO: Revision versions in config file
  bleGamepadConfig.setFirmwareRevision(FirmwareRevision); // Version of this firmware
  Serial.println("... Firmware: v" + String(FirmwareRevision));

  bleGamepadConfig.setHardwareRevision(HardwareRevision); // Version of circuit board etc.
  Serial.println("... Hardware: v" + String(HardwareRevision));

  bleGamepadConfig.setSoftwareRevision(SoftwareRevision);
  Serial.println("... Software: v" + String(SoftwareRevision));

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
  bleGamepad.begin(&bleGamepadConfig); // Note - changing bleGamepadConfig after the begin function has no effect, unless you call the begin function again
}

void SetupLittleFS()
{
  if (!LittleFS.begin(true))
  {
    // Alternative handling - Spit out an error over serial connection if problem with display
    Serial.begin(SERIAL_SPEED);
    delay(200);
    Serial.println("Critical failure, LittleFS mount failed. Halting!");
    Debug::WarningFlashes(LittleFSFailedToMount);
  }
}

void SetupComplete()
{
  // Init timestamp so its reasonably accurate for a start point, otherwise
  // initial time dependant processing may be quirky
  unsigned long currentMicroS = micros(); // + 4294967295UL - 10000000;
  FractionalSeconds = (double)(currentMicroS / 1000000.0);

  PreviousNow = Now;
  Now = FractionalSeconds + ExtendedFractionalSeconds;

  NextDisplayUpdatePoint = Now + DISPLAY_UPDATE_RATE;
}

void setup()
{
  // Development test specific
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disables brownout detector

  // delay(10); // Give pins on start up time to settle if theres any power up glitching

  Wire.begin(I2C_SDA, I2C_SCL);

  // if (!SPIFFS.begin(true))
  // {
  //   Serial.println("SPIFFS Mount Failed");
  // }

  setupDisplay();

  // Up to now, any h/w failure to initialise will be handled with flashing onboard LED warning codes.
  // Now we have a display, we can attempt to get some visuals out of any errors that take place

  setupRRE();
  setupRenderLogo();

  setupBattery();

  // We never continue if battery is too low - it's too critical - device will fail to operate soon! Spend that time warning people.
  // We check previous battery level here as once its too low its too low. No point in re-processing other stuff, and recharging involves turning off device.
  // Only in a live environment - test board might have no battery connected to produce a measurable voltage, meaning a permanent 0 battery level
  // This may change if/when decent power while play is working (and charging gets us out of this loop)
#ifdef LIVE_BATTERY
  int currentBatteryLevel = Battery::GetLevel();
  if (currentBatteryLevel == 0)
  {
    // Simulate SecondRollover and SecondFlipFlop
    // + loop forever - they need to charge things up!
    // This may change if/when decent power while play is working

    int flipFlop = false;
    while (1)
    {
      Battery::DrawEmpty(true, flipFlop, false);
      flipFlop = !flipFlop;
      delay(1000); // Wait a second
    }
  }
#endif

  setupUSB();

  // Set this up super early, used to write crashlogs etc.
  SetupLittleFS();

  // Handle potential previous crash details
  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_POWERON)
  {
    Serial.println("Debug - Device initial power on");
    Debug::PowerOnInit();
  }
  else
  {

    Serial.println("Debug - Device rebooted");
    Debug::CheckForCrashInfo();
  }

  DumpFileToSerial(Debug::CrashFile); // Output any previous crash file we might have had for info purposes

  // We need inputs set up now before anything else
  // so we can check for boot up redirection to configuration screen
  Serial.println("\nHardware...");
  Serial.println("SRam: " + String(ESP.getHeapSize()) + " (" + String(ESP.getFreeHeap()) + " free)");
  // May need compile time configure to get PSRam support - https://thingpulse.com/esp32-how-to-use-psram/?form=MG0AV3
  Serial.println("PSRam: " + String(ESP.getPsramSize()) + " (" + String(ESP.getFreePsram()) + " free)");

  // Controls icon
  RRE.drawChar(56, 50, (unsigned char)Icon_EmptyCircle_12);
  Display.display();
  delay(SETUP_DELAY / 2);
  RRE.drawChar(58, 52, (unsigned char)Icon_FilledCircle_8);
  Display.display();

  setupDigitalInputs();
  setupAnalogInputs();
  setupHatInputs();

  delay(SETUP_DELAY);

  // Check if button pushed for menu mode
  // ASSUMES BootPin_StartInConfiguration is defined as a valid pin!
  //if (digitalRead(BootPin_StartInConfiguration) == PRESSED)
  if (1==1)
  {
    MenuFunctions::Config_Setup();

    DrawConfigHelpScreen();
    Menus::Setup(&Menus::ConfigMenu);
    LoopOperation = &ConfigLoop;
    Menus::MenusStatus = ON;

    // We stay on this screen until button released
    //while (digitalRead(BootPin_StartInConfiguration) == PRESSED);

    Display.clearDisplay();
    Display.display();

    SetupComplete();

    return;
  }

  LoopOperation = &MainLoop;

  // Clear battery text ready for icons
  Display.fillRect(HALF_SCREEN_WIDTH, SCREEN_HEIGHT - RREHeight_fixed_8x16, HALF_SCREEN_WIDTH, RREHeight_fixed_8x16, C_BLACK);

#ifdef WEBSERVER
  setupWebServer();
#endif

#ifdef WIFI
  setupWiFi();
#endif

  Display.display();
  delay(SETUP_DELAY);

  setupLEDs();

  setupBluetooth();
  setupDeviceIdentifiers();

  setupController();

  Display.display();
  delay(SETUP_DELAY * 2);

  Menus::Setup(&Menus::MainMenu);

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

  SetupComplete();
}

void DrawConfigHelpScreen()
{
  Display.clearDisplay();

  /// RRE.setScale(2);

  SetFontTiny();
  SetFontLineHeightTiny();
  ResetPrintDisplayLine();
  PrintDisplayLineCenter("Device Config.");
  TextYPos += 2;
  PrintDisplayLineCenter("Navigate menu using...");
  TextYPos += 2;

  int temp = TextYPos;
  PrintDisplayLine("Up:");
  PrintDisplayLine("Down:");
  PrintDisplayLine("Select:");
  PrintDisplayLine("Back:");

  TextXPos = 60;
  TextYPos = temp;
  PrintDisplayLine(DigitalInput_Config_Up.Label);
  PrintDisplayLine(DigitalInput_Config_Down.Label);
  PrintDisplayLine(DigitalInput_Config_Select.Label);
  PrintDisplayLine(DigitalInput_Config_Back.Label);

  // Display till button is released
  SetFontFixed();
  SetFontLineHeightFixed();
  Display.display();
}

// Draw main screen
void DrawMainScreen()
{
#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Start("DrawMainScreen");
#endif

  Display.clearDisplay();

  // Controller name displayed bu default first menu option

  // SetFontFixed();
  // ResetPrintDisplayLine();
  // PrintDisplayLineCenter(DeviceName);

  SetFontCustom();
  if (ControllerGfx_RunCount > 0)
    RenderIconRuns(ControllerGfx, ControllerGfx_RunCount);

  // Whammy bar outline
  Display.drawRect(uiWhammyX, uiWhammyY, uiWhammyW, uiWhammyH, C_WHITE);

  // Bluetooth Initial
  RenderIcon(Icon_BTLogo, uiBT_xPos, uiBT_yPos, 0, 0);
  RenderIcon(Icon_EyesLeft, uiBTStatus_xPos, uiBTStatus_yPos, 0, 0);

  // Battery initial
  RenderIcon(Icon_Battery, uiBattery_xPos, uiBattery_yPos, 0, 0);

  Display.display();

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("DrawMainScreen");
#endif
}

void loop()
{
#ifdef DebugMarks
  Debug::Mark(10);
#endif

  unsigned long currentMicroS = micros(); // + 4294967295UL - 10000000;
  FractionalSeconds = (double)(currentMicroS / 1000000.0);

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
    FPS = Frame;
    Frame = 0;
  }

  // Throttle display updates
  DisplayRollover = (Now > NextDisplayUpdatePoint);
  if (DisplayRollover) {
    NextDisplayUpdatePoint += DISPLAY_UPDATE_RATE;
  }

  LoopOperation();
}

void ConfigLoop()
{
  // SetFontFixed();
  // TextYPos = 16; //ResetPrintDisplayLine();
  // PrintDisplayLineCenter("Menu stuff here");
  // Display.display();


  if (DisplayRollover || SecondRollover) {
    Menus::Config_CheckInputs();
    Menus::Handle_Config();
    Display.display();
  }
  
  Network::Config_UpdateScanResults();
}

void MainLoop()
{

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

    // UpDownCount_UpdateSubSecond();
    UpdateSubSecondStats();

    // Opportunity for a throttled battery level reading
    // Reminder that we take multiple readings and they are later averaged out
    Battery::TakeReading();

    // And finally Web traffic visuals
    Web::RenderIcons();
  }

#ifdef DebugMarks
  Debug::Mark(20);
#endif

  // Throttle LED updates
  LEDUpdateRollover = (Now > NextLEDUpdatePoint);
  if (LEDUpdateRollover)
    NextLEDUpdatePoint += LED_UPDATE_RATE;

  PreviousFractionalSeconds = FractionalSeconds;

  bool sendReport = false;

  // We never continue if battery is too low - it's too critical - device will fail to operate soon! Spend that time warning people.
  // We check previous battery level here as once its too low its too low. No point in re-processing other stuff, and recharging involves turning off device.
  // Only in a live environment - test board might have no battery connected to produce a measurable voltage, meaning a permanent 0 battery level
#ifdef LIVE_BATTERY
  if (Battery::PreviousBatteryLevel == 0)
  {
    Battery::DrawEmpty(SecondRollover, SecondFlipFlop);
    return; // Sorry - no more processing! Make em go and charge things up
  }
#endif

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.Init", showBenchmark);
#endif
#ifdef DebugMarks
  Debug::Mark(30);
#endif
  // Things we don't do so often, like every second. Reduce overhead. Would it make a difference if we didn't throttle things like this? Probably not :)
  if (SecondRollover)
  {
    // Battery stuff
    int currentBatteryLevel = Battery::GetLevel();

    // NOTE current h/w, charging is separate to powering device so can't happen at the same time. However in the future.... :)
    bool charging = false;

    // Serial.println("Battery level: " + String(currentBatteryLevel));

    if (charging)
    {
      if (SecondFlipFlop)
        LastBatteryIcon = Icon_Battery;
      else
        LastBatteryIcon = Icon_BatteryCharging;

      RenderIcon(LastBatteryIcon, uiBattery_xPos, uiBattery_yPos, 14, 11); // Actual area cleared is just the area where battery rectangle or charging lightning bolt is
    }
    else if (currentBatteryLevel == 0)
    {
      // Handling of no battery, if not using full screen handling above
      if (SecondFlipFlop)
        LastBatteryIcon = Icon_Battery;
      else
        LastBatteryIcon = Icon_BatteryEmpty;

      RenderIcon(LastBatteryIcon, uiBattery_xPos, uiBattery_yPos, 14, 11); // Actual area cleared is just the area where battery rectangle or charging lightning bolt is

      // ...however next line will trigger full screen low battery handling
      Battery::PreviousBatteryLevel = currentBatteryLevel;
    }
    else if (Battery::PreviousBatteryLevel != currentBatteryLevel)
    {
      Battery::PreviousBatteryLevel = currentBatteryLevel;
      bleGamepad.setBatteryLevel(currentBatteryLevel);
      sendReport = true;

      // Redraw standard battery icon if required
      if (LastBatteryIcon != Icon_Battery)
      {
        LastBatteryIcon = Icon_Battery;
        RenderIcon(Icon_Battery, uiBattery_xPos, uiBattery_yPos, 14, 11);
      }

      // Battery level scale to 0->10 pixels
      int width = ((currentBatteryLevel / 100.0) * 10.0);
      // Render width as 0->10 pixel wide rectangle inside icon
      Display.fillRect(uiBattery_xPos + 2, uiBattery_yPos + 3, 10, 5, C_BLACK);

      Display.fillRect(uiBattery_xPos + 2, uiBattery_yPos + 3, width, 5, C_WHITE);
    }
#ifdef DebugMarks
    Debug::Mark(40);
#endif
    // UpDownCount_SecondPassed(Second);
    UpdateSecondStats(Second);

    SetFontFixed();

    // Right text - STATS DISPLAY TEMP DISABLED
    // Display.fillRect(88, 50, 16, 16, C_BLACK);
    // PrintCenteredNumber(96, 50, Stats_StrumBar.Current_MaxPerSecond); // UpDownMaxPerSecondEver);

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
#ifdef DebugMarks
    Debug::Mark(50);
#endif
  }

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.SecondRollover", showBenchmark);
#endif

  if (SubSecondRollover)
  {
    // Update the total count more often - looks nicer
    SetFontFixed();

    // Left text Stats - STATS DISPLAY TEMP DISABLED
    // Display.fillRect(0, 50, 44, 16, C_BLACK);
    // PrintCenteredNumber(27, 50, Stats_StrumBar.Current_TotalCount); // UpDownTotalCount);
  }

  SetFontCustom();

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.SubSecondRollover", showBenchmark);
#endif

// Digital inputs
#ifdef DebugMarks
  Debug::Mark(100);
#endif
  uint16_t state;
  Input *input;

  for (int i = 0; i < DigitalInputs_Count; i++)
  {
    input = DigitalInputs[i];

    // Skip (child) digital inputs that have a LongPressParentInput - their processing will be handled indirectly via the parent
    if (input->LongPressParentInput != nullptr)
    {
      // Serial.println("Skipping " + String(input->Label) + " as it has a parent input");
      continue;
    }

    input->ValueState.StateJustChanged = false;
#ifdef DebugMarks
    Debug::Mark(110);
#endif
    unsigned long timeCheck = micros();
    if ((timeCheck - input->ValueState.StateChangedWhen) > DEBOUNCE_DELAY)
    {
      // Compare current with previous, and timing, so we can de-bounce if required
      state = digitalRead(input->Pin);

      int skipCheck = false;

      // Digital inputs are easy, EXCEPT when using time delays
      // e.g. you might have a Select button, however if that Select button is held down for more than 1 second, rather than
      // delivering a Select button press, it ignores that and instead calls some custom code to e.g. change the controller mode to go to custom
      // controller menu

      // Bluetooth press/release operations should be paired nicely to keep on top of a proper controller state
      // so we can't just send a Bluetooth press, wait a bit to see what release happens - might not be wanted

      // if has linked input then
      if (input->LongPressChildInput != NONE)
      {
        unsigned long timeDifference = timeCheck - input->ValueState.StateChangedWhen;
        // Serial.println("NAME - " + String(input->Label));
        // Serial.println("State - " + String(state) + " for " + String(input->ValueState.Value));
#ifdef DebugMarks
        Debug::Mark(120);
#endif
        if (state == PRESSED)
        {
          if (input->ValueState.Value != LONG_PRESS_MONITORING)
          {
            // Serial.println("LONG PRESS - MONITORING INITIATED");

            input->ValueState.Value = LONG_PRESS_MONITORING;
            input->ValueState.StateChangedWhen = timeCheck;
          }
          else if (timeDifference >= input->LongPressTiming)
          {
            // Serial.println("LONG PRESSED TIMING TRIGGER " + String(timeDifference) + " vs " + String(input->LongPressTiming));

            // Past long press time, pass child on to main routine
            input = input->LongPressChildInput;
          }
          else
          {
            // Serial.println("Waiting " + String(timeDifference) + " vs " + String(input->LongPressTiming));

            // Render icon to screen for initial press we MIGHT have, only a % visible in relation
            // to length of time held to total length of time before becomes long press input
            input->RenderOperation(input);
            float percentage = static_cast<float>(timeDifference) / static_cast<float>(input->LongPressTiming);
            RenderInput_BlankingArea(input, percentage);
          }
        }
        else // if (state == NOT_PRESSED)
        {
          if (input->ValueState.Value == LONG_PRESS_MONITORING)
          {
            if (timeDifference >= input->LongPressTiming)
            {
              // We were monitoring a long press, so we need to release the child input

              // Serial.println("LONG PRESS - ESCALATING TO CHILD");
              //  first, de-escalate the long press monitoring
              input->ValueState.Value = NOT_PRESSED;

              // pass along child input to have its release processed
              input = input->LongPressChildInput;

              // Serial.println("LONG PRESS - LONG PRESSED");
            }
            else
            {
              // This was a short press, so we need to trigger the primary input
              input->ValueState.Value = NOT_PRESSED; // Will force a press when we process the input below
              state = PRESSED;                       // Force a pretend pressing for this cycle for this input
              // Next loop will pick up it is not pressed any more and do the release
              // Serial.println("LONG PRESS - SHORT PRESSED");

              input->AutoHold = timeCheck + input->ShortPressReleaseTime;
            }
          }

          // Sorts out holding button after released for a time frame - means
          // short press triggers can be for a bigger time period for things
          // like UI components and LED's to show long enough the user can see them
          if (input->AutoHold > timeCheck)
          {
            state = PRESSED;
          }
        }
      }
#ifdef DebugMarks
      Debug::Mark(130);
#endif
      // Process when state has changed
      if (state != input->ValueState.Value && input->ValueState.Value != LONG_PRESS_MONITORING)
      {
#ifdef DebugMarks
        Debug::Mark(140);
#endif
        // Serial.println("Digital Input Changed: " + String(input->Label) + " to " + String(state));
        input->ValueState.PreviousValue = !state;
        input->ValueState.Value = state;
        input->ValueState.StateChangedWhen = timeCheck;
        input->ValueState.StateJustChanged = true;
        input->ValueState.StateJustChangedLED = true;

        // Digital inputs are easy, EXCEPT when using time delays
        // e.g. you might have a Select button, however if that select button is held down for more than 1 second, rather than
        // delivering a Select button press, it ignores that and instead calls some custom code to e.g. change the controller mode to go to custom
        // controller menu

        // Bluetooth press/release operations should be paired nicely to keep on top of a proper controller state
        // so we can't just send a Bluetooth press, wait a bit to see what release happens...

        // If input->DelayedPressTiming > 0 then we don't do a press until the button is pressed and then released again before the press time happens
        // else we treat it normally

        // A delayed operation...
        // Finding it out
        // The digital input we pick up there is a delayed operation
        // if its NOT triggered, then we flag here as a Press and Un-press at the same time
        // AND set a `do the led's anyway` flag so the LED code can run for 1 loop
        // If it IS triggered, then we just pass on the fact there is an input press going on based on the timing as if the timing zeros itself
        // on the press length. That way the long press input should act like a normal input and can be processed as such.
        // The secondary control should know no better!
        // Theoretically we can chain them together maybe?

        // OK SO if delay time ISN'T met and we want to do an instant press release
        // check for state == low, and if there was a delay timer then
        // basically treat state == gone low + timer < delay = high, then next time through the state = high + timer < delay is time to go low
        // MAY need an extra state = DELAY that lets us know we are delay checking until eventually it goes high or gets passed on

        // We have to check input states not just for Bluetooth inputs, as buttons might control device functionality

        ControllerReport customOperationControllerReport = ReportToController;

        if (state == PRESSED)
        {
          // PRESSED!

          // Any extra special custom to specific controller code
          if (input->CustomOperationPressed != NONE)
            customOperationControllerReport = input->CustomOperationPressed();

          if (customOperationControllerReport == ReportToController)
          {
            if (input->BluetoothInput != 0)
            {

              // bleGamepad.press(input->BluetoothInput);
              (bleGamepad.*(input->BluetoothPressOperation))(input->BluetoothInput);

              sendReport = true;
            }

            // Statistics
            if (input->Statistics != nullptr)
              input->Statistics->AddCount();
          }
        }
        else
        {
          // RELEASED!!
#ifdef DebugMarks
          Debug::Mark(150);
#endif
          // Any extra special custom to specific controller code
          if (input->CustomOperationReleased != NONE)
            customOperationControllerReport = input->CustomOperationReleased();

          if (customOperationControllerReport == ReportToController)
          {
            if (input->BluetoothInput != 0)
            {
              // bleGamepad.release(input->BluetoothInput);
              (bleGamepad.*(input->BluetoothReleaseOperation))(input->BluetoothInput);

              sendReport = true;
            }
          }
        }

        input->RenderOperation(input);
      }
    }
  }

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.DigitalInputs", showBenchmark);
#endif

// Analog Inputs
#ifdef DebugMarks
  Debug::Mark(200);
#endif
  for (int i = 0; i < AnalogInputs_Count; i++)
  {
    input = AnalogInputs[i];
    state = analogRead(input->Pin);
#ifdef DebugMarks
    Debug::Mark(210);
#endif
    // We only register a change if it is above a certain level of change (kind of like smoothing it but without smoothing it)
    float threshold = .03 * 4095; // 3% change required
    int previousValue = input->ValueState.Value;
    if (state < (previousValue - threshold) || state > (previousValue + threshold))
    {
      if (state != input->ValueState.Value)
      {
#ifdef DebugMarks
        Debug::Mark(220);
#endif
        input->ValueState.Value = state;
        input->ValueState.PreviousValue = previousValue;
        input->ValueState.StateChangedWhen = micros(); // ToDo: More accurate setting here, as there have been delays

        int16_t minValue = input->MinValue;
        int16_t maxValue = input->MaxValue;
        int16_t constrainedState = constrain(state, minValue, maxValue);

        int16_t rangedState = map(constrainedState, minValue, maxValue, 0, 32737); // Scale range to match Bluetooth range. Ranged state min/max theoretically 0->4095

        // Serial.println("Constrained " + String(constrainedState) + " - Ranged; " + String(rangedState));

        (bleGamepad.*(input->BluetoothSetOperation))(rangedState);

        input->RenderOperation(input);

        // Any extra special custom to specific controller code
        if (input->CustomOperationPressed != NONE)
          input->CustomOperationPressed();

        sendReport = true;
#ifdef DebugMarks
        Debug::Mark(230);
#endif
      }
    }

    // Analog effects are constantly calculated, shifted to other core for performance
  }

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.AnalogInputs", showBenchmark);
#endif

  // Hat inputs
#ifdef DebugMarks
  Debug::Mark(300);
#endif

  // bool hatChanged = false;
  int hatPreviousState;
  int hatCurrentState;
  int subState;
  // Hat's assume all pins are read. Will need to adjust code if this is not the case
  HatInput *hatInput;
  unsigned long timeCheck;

  for (int i = 0; i < HatInputs_Count; i++)
  {
#ifdef DebugMarks
    Debug::Mark(310);
#endif

    hatInput = HatInputs[i];
    hatInput->ValueState.StateJustChanged = false;

    hatPreviousState = hatInput->ValueState.Value;

    // Step 1, read in current states
    for (int j = 0; j < 4; j++)
    {
      // Not forgetting to account for debouncing

      // timeCheck = micros();

      if ((micros() - hatInput->IndividualStateChangedWhen[j]) > DEBOUNCE_DELAY)
      {
        // Might not be using all pins
        uint8_t pin = hatInput->Pins[j];
        if (pin != NONE)
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
#ifdef DebugMarks
    Debug::Mark(320);
#endif
    // Step 2, copy first pin state to extra buffer pin in states (faster/easier calculations, no wrapping needed)
    hatInput->IndividualStates[4] = hatInput->IndividualStates[0];

    hatCurrentState = HAT_CENTERED; // initially nothing
    subState = 1;                   // First possible value with something there (1 = Up. 0 = neutral/nothing position)

    // Special case
    if (hatInput->IndividualStates[0] == PRESSED && hatInput->IndividualStates[3] == PRESSED)
      hatCurrentState = 8;
    else
    {
#ifdef DebugMarks
      Debug::Mark(330);
#endif

      // Step 3, check values accounting for 2 press diagonals. We go backwards so other states are checked before the 0 button, which lets us check the 3+0 button combination extra buffer thingy
      for (int j = 0; j < 4; j++)
      {
        if (hatInput->IndividualStates[j] == PRESSED) // Pressed
        {
          hatCurrentState = subState;

          // Check the diagonal by checking next pin (i.e. both pins pressed)
          if (hatInput->IndividualStates[j + 1] == PRESSED)
            hatCurrentState++; // also pressed, so make diagonal

          break; // Bail from loop - no point in checking for further pressed keys, should be physically impossible on a hat (or be irrelevant)
        }

        subState += 2; // Hat values go 1, 3, 5, 7 for Up, Right, Down, Left, diagonals go 2, 4, 6, 8
      }
    }
#ifdef DebugMarks
    Debug::Mark(340);
#endif
    // Final check to see if things have changed since last time
    // Serial.print("Hat current state: " + String(hatPreviousState) + "" - New State: " + String(hatCurrentState));

    if (hatCurrentState != hatPreviousState)
    {
#ifdef DebugMarks
      Debug::Mark(350);
#endif
      hatInput->ValueState.Value = hatCurrentState;
      hatInput->ValueState.PreviousValue = hatPreviousState;
      hatInput->ValueState.StateChangedWhen = micros(); // TODO make this when hat last read took place though realistically bog all time will have passed
      hatInput->ValueState.StateJustChangedLED = true;

      // hatChanged = true;

      hatInput->RenderOperation(hatInput);

      // Any general hat level extra operations
      if (hatInput->CustomOperation != NONE)
        hatInput->CustomOperation(hatInput);

      ControllerReport extraOperationControllerReport = ReportToController; // DontReportToController;
                                                                            // Hat position specific extra operations
                                                                            // We allow cancellation of bluetooth setting here so
                                                                            // we can use hat activities for onboard operations (such as menu navigation)
                                                                            // without it being reported back via bluetooth
#ifdef DebugMarks
      Debug::Mark(360);
#endif
      if (hatInput->ExtraOperation[hatCurrentState] != NONE)
        extraOperationControllerReport = hatInput->ExtraOperation[hatCurrentState]();

      if (extraOperationControllerReport == ReportToController)
      {
        hatInput->ValueState.StateJustChanged = true;

        // General hat array used with bluetooth library
        HatValues[hatInput->BluetoothHat] = hatCurrentState;

        // Statistics
        // We don't count statistics if bluetooth operation was cancelled
        if (hatInput->Statistics[hatCurrentState] != nullptr)
          hatInput->Statistics[hatCurrentState]->AddCount();
      }
    }
#ifdef DebugMarks
    Debug::Mark(370);
#endif
  }

  if (hatInput->ValueState.StateJustChanged)
  { // hatChanged) {
#ifdef EXTRA_SERIAL_DEBUG
    Serial.print("Hat Change: ");
    Serial.println(HatValues[0]);
#endif
#ifdef DebugMarks
    Debug::Mark(380);
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

  // Little generalised check to see if anything has changed
  if (sendReport)
    LastTimeAnyButtonPressed = Now;

  // Call idle LED effects etc. if controllers not had anything pressed for a while
  if (Now - LastTimeAnyButtonPressed > IDLE_TIMEOUT)
    ControllerIdle = true;
  else
  {
    if (ControllerIdle)
    {
      ControllerIdleJustUnset = true;
      ControllerIdle = false;
    }
  }
#ifdef DebugMarks
  Debug::Mark(400);
#endif
  // Bluetooth
  BTConnectionState = bleGamepad.isConnected();

  if (BTConnectionState == true)
  {
#ifdef DebugMarks
    Debug::Mark(410);
#endif
    if (sendReport)
    {
      bleGamepad.sendReport();

#ifdef EXTRA_SERIAL_DEBUG_PLUS
      Serial.println(String(Frame) + ": BT Report Sent");
#endif
    }

    // Check if BT connection state changed at all
    if (PreviousBTConnectionState != BTConnectionState)
    {
      // Draw the connected icon
      RenderIcon(Icon_OK, uiBTStatus_xPos, uiBTStatus_yPos, 16, 12);
      PreviousBTConnectionState = BTConnectionState;

#ifdef USE_EXTERNAL_LED
      // Remove any BT LED status
      ExternalLedsEnabled[ExternalLED_StatusLED] = false;
#endif
    }
#ifdef DebugMarks
    Debug::Mark(420);
#endif
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
#ifdef DebugMarks
    Debug::Mark(430);
#endif

    if (SecondRollover)
      RenderIcon(SecondFlipFlop ? Icon_EyesLeft : Icon_EyesRight, uiBTStatus_xPos, uiBTStatus_yPos, 16, 12); // Blanking area big enough to cover the `OK` if required

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
#ifdef DebugMarks
  Debug::Mark(500);
#endif
  PreviousBTConnectionState = BTConnectionState;

#ifdef WIFI
  if (SecondRollover)
    Network::HandleWiFi(Second);
#endif
#ifdef DebugMarks
  Debug::Mark(550);
#endif
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
#ifdef DebugMarks
  Debug::Mark(600);
#endif
#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.LEDStuff", showBenchmark);
#endif

  if (DisplayRollover || SecondRollover)
    Menus::HandleMain();

#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.MenuHandled", showBenchmark);
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
#ifdef DebugMarks
  Debug::Mark(700);
#endif
  Frame++;
// Serial.println("Frame: " + String(Frame));
#ifdef INCLUDE_BENCHMARKS
  MainBenchmark.Snapshot("Loop.Display", showBenchmark);
#endif
}
