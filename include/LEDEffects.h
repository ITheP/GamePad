#pragma once

#include "FastLED.h"

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

class DigitalArrayEffects
{
public:
  static void Rain(void *digitalInput, float time);
  static void BlendedRain(void *digitalInput, float time);
  static void Sparkle(void *digitalInput, float time);
  static void SparkleTimeHue(void *digitalInput, float time);
  static void SparkleTimeBlend(void *digitalInput, float time);
};

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

class AnalogArrayEffects
{
public:
  static void SetSingleAlongArray(void *analogInput, float time);
  static void FillToPointAlongArray(void *analogInput, float time);
  static void BuildBlendToEnd(void *analogInput, float time);
  static void SquishBlendToPoint(void *analogInput, float time);
  static void PointWithTail(void *analogInput, float time);
};