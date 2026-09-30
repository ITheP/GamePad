#include "DisplayEffects.h"
#include <Adafruit_GFX.h> // Required for Display object
#include "Screen.h"

// Initialize static members
Pixel DisplayEffects::gleamPixels[MAX_GLEAM_PIXELS];
int DisplayEffects::gleamCount = 0;

// Pretty looking glint effect that streaks across the screen
void DisplayEffects::RenderGlint(int frame, int height, int width)
{
  // // Step 1: Reset logo from any previous frame
  // Display.clearDisplay();
  // RenderLogoRuns(Logo, Logo_RunCount);

  // Step 2: Scan diagonal line
  gleamCount = 0;
  int x = -height + frame;

  for (int y = height; y > 0; --y)
  {
    if (x >= 0 && x < width)
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
}