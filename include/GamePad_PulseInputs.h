#pragma once

#include "Structs.h"

void setupPulseInputs();
void mcpwm_capture_callback();

int measureDutyCycle(PulseInput *pulseInput);
void updatePulseInputs();
extern volatile int TestBob;

extern volatile uint32_t s_captured_period;
extern volatile uint32_t s_captured_high;
extern volatile uint32_t s_last_timestamp;
extern volatile bool s_is_high;