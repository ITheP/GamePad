#pragma once

#include <Arduino.h>

float fmap(float x, float in_min, float in_max, float out_min, float out_max);
const char* getBuildVersion();
void DumpFileToSerial(const char* path);
String SaferPasswordString(String password);
String SaferShortenedPasswordString(String password);