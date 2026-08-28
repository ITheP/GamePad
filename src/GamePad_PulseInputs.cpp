#include "GamePad_PulseInputs.h"
#include <Structs.h>
#include <Debug.h>
#include <DeviceConfig.h>

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "driver/gpio.h"
#include <soc/soc.h>

// Data we are playing with - MUST be in DRAM else our interrupt is not going to be happy

// Pointer to the dynamically allocated capture data array
// This will point to DRAM-allocated memory
PulseCaptureData_t* g_pulse_capture_data = NULL;

// ISR - must be in IRAM and use only DRAM data
static bool IRAM_ATTR capture_isr(
    mcpwm_unit_t mcpwm, 
    mcpwm_capture_channel_id_t cap_channel, 
    const cap_event_data_t *edata, 
    void *user_data)
{
    int channel_index = (int)(uintptr_t)user_data;
    
    if (channel_index < 0 || g_pulse_capture_data == NULL) {
        return false;
    }
    
    PulseCaptureData_t *data = &g_pulse_capture_data[channel_index];
    uint32_t current = edata->cap_value;

    // Check for timer rollover (if current < previous, timer wrapped)
    if (data->last_timestamp != 0 && current < data->last_timestamp) {
        // Timer rolled over - invalidate everything and reset
        data->has_valid_period = false;
        data->last_rising_edge = 0;
        data->last_falling_edge = 0;
        data->captured_period = 0;
        data->captured_high = 0;
        data->last_timestamp = current;
        //data->is_high = false;
        return false;  // ⚠️ EXIT EARLY - don't process this edge
    }

    if (edata->cap_edge == MCPWM_POS_EDGE) {
        if (data->last_rising_edge != 0) {
            data->captured_period = current - data->last_rising_edge;
            // Only mark valid if period is reasonable (e.g., > 10 ticks to avoid noise)
            if (data->captured_period > 10) {
                data->has_valid_period = true;
            } else {
                data->has_valid_period = false;  // Too short - likely noise
            }
        } else {
            data->has_valid_period = false;
        }
        data->last_rising_edge = current;
        //data->is_high = true;
    } 
    else if (edata->cap_edge == MCPWM_NEG_EDGE) {
        if (data->last_rising_edge != 0 && current >= data->last_rising_edge) {
            data->captured_high = current - data->last_rising_edge;
            // Verify high time is less than period (sanity check)
            if (data->has_valid_period && data->captured_high > data->captured_period) {
                data->has_valid_period = false;  // Invalid - high > period
            }
        } else {
            data->captured_high = 0;
            data->has_valid_period = false;
        }
        data->last_falling_edge = current;
        //data->is_high = false;
    }
    
    data->last_timestamp = current;
    return false;
}

// Helper function to free allocated memory (call if setup fails)
static void free_capture_data() {
    if (g_pulse_capture_data != NULL) {
        free((void*)g_pulse_capture_data);
        g_pulse_capture_data = NULL;
    }
}

void setupPulseInputs() {
    Serial.println();
    Serial.println("🎚 Pulse Input via Hardware MCPWM - Count: " + String(PulseInputs_Count));

    if (PulseInputs_Count < 1) {
        Serial.println("❌ No pulse inputs configured!");
        return;
    }

    // Dynamically allocate capture data array in DRAM
    // Use heap_caps_malloc to ensure it's in DRAM (not PSRAM)
    size_t alloc_size = sizeof(PulseCaptureData_t) * PulseInputs_Count;
    g_pulse_capture_data = (PulseCaptureData_t*)heap_caps_malloc(alloc_size, MALLOC_CAP_INTERNAL);

    if (g_pulse_capture_data == NULL) {
        Serial.printf("❌ Failed to allocate %d bytes for capture data!\n", alloc_size);
        return;
    }
    
    // Zero-initialize all capture data
    memset((void*)g_pulse_capture_data, 0, alloc_size);

    esp_err_t err;
    
    // Store count in a global for ISR safety checks
    // (PulseInputs_Count is already global from Structs.h)

    // Configure each pulse input
    for (int i = 0; i < PulseInputs_Count; i++) {
        PulseInput *sourceInput = PulseInputs[i];
        int pin = sourceInput->Pin;
        
        // Store the index in the pulse input structure for later reference
        sourceInput->MCPWMIndex = i;
        //sourceInput->HasValidPeriod = false;
        //sourceInput->Frequency = 0;

        // Configure GPIO
        gpio_reset_pin((gpio_num_t)pin);
        gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
        gpio_pullup_en((gpio_num_t)pin);

        // Map MCPWM unit and channel based on index
        mcpwm_unit_t unit = MCPWM_UNIT_0;
        mcpwm_timer_t timer = MCPWM_TIMER_0;

        // For mcpwm_gpio_init - use MCPWM_CAP_0, MCPWM_CAP_1, MCPWM_CAP_2
        mcpwm_io_signals_t cap_signal;
        switch (i % 3) {
            case 0: cap_signal = MCPWM_CAP_0; break;
            case 1: cap_signal = MCPWM_CAP_1; break;
            case 2: cap_signal = MCPWM_CAP_2; break;
            default: cap_signal = MCPWM_CAP_0; break;
        }
        
        // For mcpwm_capture_enable_channel - use MCPWM_SELECT_CAP0, etc.
        mcpwm_capture_signal_t cap_channel;
        switch (i % 3) {
            case 0: cap_channel = MCPWM_SELECT_CAP0; break;
            case 1: cap_channel = MCPWM_SELECT_CAP1; break;
            case 2: cap_channel = MCPWM_SELECT_CAP2; break;
            default: cap_channel = MCPWM_SELECT_CAP0; break;
        }

        // Initialize MCPWM timer only once
        if (i == 0) {
            mcpwm_config_t timer_config = {
                .frequency = 1000000,
                .cmpr_a = 0,
                .cmpr_b = 0,
                .duty_mode = MCPWM_DUTY_MODE_0,
                .counter_mode = MCPWM_UP_COUNTER,
            };
            err = mcpwm_init(unit, timer, &timer_config);
            if (err != ESP_OK) {
                Serial.printf("❌ MCPWM init failed: %s\n", esp_err_to_name(err));
                free_capture_data();
                return;
            }
        }

        // Route GPIO to the capture pin - uses mcpwm_io_signals_t
        err = mcpwm_gpio_init(unit, cap_signal, pin);
        if (err != ESP_OK) {
            Serial.printf("❌ GPIO init failed for pin %d: %s\n", pin, esp_err_to_name(err));
            continue;
        }

        // Pass the index as user_data so ISR knows which channel this is
        mcpwm_capture_config_t capture_config = {
            .cap_edge = MCPWM_BOTH_EDGE,
            .cap_prescale = 1,
            .capture_cb = capture_isr,
            .user_data = (void *)(uintptr_t)i
        };

        // Enable capture channel - uses mcpwm_capture_signal_t
        err = mcpwm_capture_enable_channel(unit, cap_channel, &capture_config);

        if (err != ESP_OK) {
            Serial.printf("❌ Capture enable failed for pin %d: %s\n", pin, esp_err_to_name(err));
            continue;
        }

        Serial.printf("✅ Hardware MCPWM Capture ready on pin %d (channel %d)\n", pin, i);
    }

    Serial.println("✅ All pulse inputs configured!");
}