#pragma once

#include <RREFont.h>
#include "Structs.h"

void FlipScreen(Input* input);
void DrawMainScreen();
void DrawEmpty();

extern RREFont RRE;
extern float Now;

extern char SerialNumber[];
extern char DeviceName[];
extern const char* DeviceNames[];
extern int DeviceNamesCount;