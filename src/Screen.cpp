#include "Screen.h"
#include "Stats.h"
#include "Battery.h"

Adafruit_SH1106G Display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Screen orientation functions
int ScreenOrientation = 0;
int PreviousScreenOrientation = 0;

void FlipScreen(Input* input) {
#ifdef LIVE_BATTERY
  // Don't draw if we are showing the end game battery warning
  if (Battery::PreviousBatteryLevel == 0) {
#ifdef EXTRA_SERIAL_DEBUG
    Serial.println("Flip Screen cancelled - BatteryLevel 0");
#endif
    return;
  }
#endif

// #ifdef EXTRA_SERIAL_DEBUG
//   Serial.println("Flip Screen " + String(input->ValueState.Value));
// #endif

  // Set orientation will also need flipping

  // Note: Define FLIP_SCREEN_TOGGLE in controller config .h if you want a flip screen button to toggle each press, rather than a button hold to toggle screen orientation
  // .setRotation does not require Display.display() to be called
  
#ifdef FLIP_SCREEN_TOGGLE
  if (input->Value == HIGH)
    return;

  if (ScreenOrientation == 2)
    ScreenOrientation = 0;
  else
    ScreenOrientation = 2;

  Display.setRotation(ScreenOrientation);
#else
  if (input->ValueState.Value == HIGH)
    ScreenOrientation = 0;
  else
    ScreenOrientation = 2;

  Display.setRotation(ScreenOrientation);
#endif

  if (ScreenOrientation != PreviousScreenOrientation) {
    // Flip current content
    // Note that the main DrawMainScreen() is only partial in what it draws - doesn't include all the individual control points and dynamic values.
    // So we have a little (slow but functional) screen flipping routine to make sure what is already there is copied exactly as is, rather than clearing and redrawing the main screen.

    // We only need to cover 1/2 the screen, as it is swapping places with the other half.
    int height = SCREEN_HEIGHT >> 1;

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < SCREEN_WIDTH; x++) {
        // Read the pixel starting from the top-left corner
        int topPixel = Display.getPixel(x, y);

        // Read the pixel we are swapping with from the bottom-right corner
        int bottomPixel = Display.getPixel(SCREEN_WIDTH - 1 - x, SCREEN_HEIGHT - 1 - y);

        // Only drawing where there are actual differences saves a bit of time
        if (topPixel != bottomPixel) {
          // Draw the bottom pixel to the top position
          Display.drawPixel(x, y, bottomPixel);

          // Draw the top pixel to the bottom position
          Display.drawPixel(SCREEN_WIDTH - 1 - x, SCREEN_HEIGHT - 1 - y, topPixel);
        }
      }
    }
    // Update the display to show the changes
    Display.display();

    PreviousScreenOrientation = ScreenOrientation;
  }

  // Finally, if required, clear statistics when screen flips
// #ifdef CLEAR_STATS_ON_FLIP
//   Stats_StrumBar.ResetCurrentCounts();
// #endif
}