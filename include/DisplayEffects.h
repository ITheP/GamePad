#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h> // Assuming Display uses this or similar graphics library

constexpr int MAX_GLEAM_PIXELS = 128;

// Structure to hold gleam pixel coordinates
struct Pixel
{
  uint8_t x;
  uint8_t y;
};

class DisplayEffects
{
public:
    // Static method to render the glint effect
    static void RenderGlint(int frame, int height, int width);

private:
    // Static storage for gleam pixels and count
    static Pixel gleamPixels[MAX_GLEAM_PIXELS];
    static int gleamCount;
};