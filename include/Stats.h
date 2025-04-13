#pragma once

#include "Config.h"
#include "Structs.h"

#define UpDownCounts_Count 60                       // 60 second across a minute count
#define UpDownSubCounts_Count (SubSecondCount * 2)  // Overall size of window that a 1 second sliding window is checked across to find out the actual max per second counts
//#define SubSecondCount 30                           // Number of sub-samples to take across a second when calculating clicks per second counts. Higher number = more accurate but more overhead. Also used as frequency for lower rate refreshes.

extern int UpDownCurrentCount;
extern int UpDownCounts[];
extern int UpDownSubCounts[];
extern int UpDownSubCountPos;

// Stats
extern long UpDownTotalCount;
extern long UpDownMaxPerSecondCount;
extern long UpDownMaxPerSecondOverLastMinuteCount;
extern long UpDownMaxPerSecondEver;

void UpDownCount_UpdateSubSecond();
void UpDownCount_SecondPassed(int Second);
void UpDownCountClick(HatInput* input);
void UpDownCount_Reset();