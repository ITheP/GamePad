#pragma once

#include "Structs.h"

void setupPulseInputs();
void mcpwm_capture_callback();

int measureDutyCycle(PulseInput *pulseInput);
void updatePulseInputs();