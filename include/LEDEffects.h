#pragma once

#include "FastLED.h"

// Digital Input triggered effects, applied to individual LED
class DigitalEffects
{
public:
  static void Test(void *digitalInput, float time);
  static void SimpleSet(void *digitalInput, float time);
  static void Throb(void *digitalInput, float time); // Colour moves from Secondary to Primary colour and then loops back around
                                                     // ExternalLEDRate = speed of throb i.e. (uint8_t)(Now * ExternalLEDRate) == 0 -> 255
  static void Pulse(void *digitalInput, float time);
  static void TimeHue(void *digitalInput, float time);
  static void MoveRainbow(void *digitalInput, float time);
};

// Digital Input triggered effects, applied to an array of LED's
class DigitalArrayEffects
{
public:
  static void Rain(void *digitalInput, float time);
  static void BlendedRain(void *digitalInput, float time);
  static void Sparkle(void *digitalInput, float time);
  static void SparkleTimeHue(void *digitalInput, float time);
  static void SparkleTimeBlend(void *digitalInput, float time);
};

// HAT Input triggered effects
class HatEffects
{
public:
  static void Test(void *hatInput, float time);
  static void SimpleSet(void *hatInput, float time);
  static void Throb(void *hatInput, float time); // Colour moves from Secondary to Primary colour and then loops back around
                                                 // ExternalLEDRate = speed of throb i.e. (uint8_t)(Now * ExternalLEDRate) == 0 -> 255
  static void Pulse(void *hatInput, float time);
  static void TimeHue(void *hatInput, float time);
  static void MoveRainbow(void *hatInput, float time);
};

// Analog Input triggered effects, applied to individual LED
class AnalogEffects
{
public:
  static void Test(void *analogInput, float time);
  static void SimpleSet(void *analogInput, float time);
  static void Throb(void *analogInput, float time); // Colour moves from Secondary to Primary colour and then loops back around
                                                    // ExternalLEDRate = speed of throb i.e. (uint8_t)(Now * ExternalLEDRate) == 0 -> 255
  static void BlendedHue(void *analogInput, float time);
  static void Hue(void *analogInput, float time);
  static void StartAtHue(void *analogInput, float time);
  static void EndAtHue(void *analogInput, float time);
  static void BlendedStartAtHue(void *analogInput, float time);
  static void BlendedEndAtHue(void *analogInput, float time);
};

// Analog Input triggered effects, applied to an array of LED's
class AnalogArrayEffects
{
public:
  static void SetSingleAlongArray(void *analogInput, float time);
  static void FillToPointAlongArray(void *analogInput, float time);
  static void BuildBlendToEnd(void *analogInput, float time);
  static void SquishBlendToPoint(void *analogInput, float time);
  static void PointWithTail(void *analogInput, float time);
};

// General effects, that run independent to any input
class GeneralArrayEffects
{
public:
  // Uses Stats linked to the LEDConfig to determine the percentage of the LED's that should be lit
  static void Elastic(void* ExternalLEDConfig, float time);

  static void Random(void* ExternalLEDConfig, float time);

  // Used to disable all LED's e.g. end of Idle state so everything fades out
  static void DisableAll(void* ExternalLEDConfig, float time);
};