#include "GamePad_Profile.h"
#include <Structs.h>
#include <Debug.h>
#include <DeviceConfig.h>
#include "Screen.h"
#include "RenderText.h"
#include "Icons.h"
#include "LED.h"
#include <Profiles.h>
#include <Networking.h>
#include <Adafruit_SSD1306.h>

void setupDisplay()
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

  Serial_INFO;
  Serial.println("📺 Setting up display...");

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // if (!Display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, &Wire, OLED_RESET)) {

#ifdef SCREEN_INTERFACE_I2C
  if (!Display.begin(SCREEN_ADDRESS, true))
#else
  SPI.begin(SCREEN_SPI2_SCK_PIN, -1, SCREEN_SPI2_MOSI_PIN); // -1 as no MISO required
  if (!Display.begin(SSD1306_SWITCHCAPVCC))
#endif
  {
#ifdef REQUIRE_DISPLAY
    Serial_ERROR;
    Serial.println("⛔ Critical failure, display failed to start. Halting!");
    Debug::WarningFlashes(LittleFSFailedToMount);
#else
    Serial_ERROR;
    Serial.println("⛔ Critical failure, display failed to start. This will be ignored as REQUIRE_DISPLAY is not defined, but be aware this cause unexpected issues.");
#endif
  }

  // Display.invertDisplay(true);

  // #ifdef SCREEN_INTERFACE_I2C
  // // This MIGHT be required if using SH1106 display with SSD1306 as driver
  // //Display.setCursor(2, 0);
  // #endif

  Display.clearDisplay();
  Display.display();
}