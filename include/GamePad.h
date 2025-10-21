#pragma once

#include <RREFont.h>
#include "Structs.h"

#define COREVERSION "1.0"        // The core version of this software.
                                 // Individual controller code, firmware, hardware versions are also recorded separately in the dedicated controller config files    

void FlipScreen(Input* input);
void DrawMainScreen();
void DrawEmpty();

extern RREFont RRE;
extern float Now;

extern int PreviousSecond;
extern int Second;
extern int SecondRollover; // Flag to easily sync operations that run once per second
extern int SecondFlipFlop; // Flag flipping between 0 and 1 every second to allow for things like blinking icons

extern int PreviousSubSecond;
extern int SubSecond;
extern int SubSecondRollover; // SubSecond flag for things like statistics sampling
extern int SubSecondFlipFlop;

extern int Frame;
extern int FPS;

extern int DisplayRollover;

extern char SerialNumber[];
extern char DeviceName[];
extern const char* DeviceNames[];
extern int DeviceNamesCount;
extern int ControllerIdle;
extern int ControllerIdleJustUnset;
extern bool BTConnectionState;