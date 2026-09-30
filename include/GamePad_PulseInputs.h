#pragma once

#include "Structs.h"

void setupPulseInputs();
void mcpwm_capture_callback();

int measureDutyCycle(PulseInput *pulseInput);
void updatePulseInputs();

extern volatile uint32_t s_captured_period;
extern volatile uint32_t s_captured_high;
extern volatile uint32_t s_last_timestamp;
extern volatile bool s_is_high;

// Each pulse input channel gets its own capture data
typedef struct {
    volatile uint32_t last_rising_edge;
    volatile uint32_t last_falling_edge;
    volatile uint32_t captured_period;
    volatile uint32_t captured_high;
    volatile uint32_t last_timestamp;
    //volatile bool is_high;
    volatile bool has_valid_period;
} PulseCaptureData_t;

// Array of capture data structures - one per pulse input
// Declared as extern so it can be accessed from other files if needed
extern PulseCaptureData_t* g_pulse_capture_data;