#pragma once

#include <RREFont.h>
#include "Structs.h"

#define COREVERSION "1.0"        // The core version of this software.
                                 // Individual controller code, firmware, hardware versions are also recorded separately in the dedicated controller config files    

                                 // Keep the About message < 256 characters
#define ABOUT "The Controller - multi-purpose controller software primarily to bodge and duct tape old legacy controllers back into life or make new ones in weird and wacky ways! Connect to the embedded web server for more information."

void FlipScreen(Input* input);

void DrawMainScreen();
void MainLoop();
void DrawMenuScreen();
void MenuLoop();
void DrawConfigHelpScreen();
void ConfigLoop();
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
extern char FullDeviceName[];
extern const char* DeviceNames[];
extern int DeviceNamesCount;
extern int ControllerIdle;
extern int ControllerIdleJustUnset;
extern bool BTConnectionState;