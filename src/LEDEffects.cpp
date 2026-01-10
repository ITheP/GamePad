#include "Structs.h"
#include "LED.h"
#include "LEDEffects.h"

extern int ExternalLedsEnabled[];

// NOTES:
// FastLED does have faster routines than some of our DIY jobs
// however our array's of LED's may be in any order (rather than continuous allocations of memory) and have other bits and bobs going on which doesn't fit the FastLED standard
// so our custom processing here does the job.
// Calls are also throttled.

// FastLED loves its fract8 etc. datatype for fast floats - however some of the tests here where we were using fractions
// ended up with non-smooth transitions - so we float it!