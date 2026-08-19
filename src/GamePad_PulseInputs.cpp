#include "GamePad_PulseInputs.h"
#include <Structs.h>
#include <Debug.h>
#include <DeviceConfig.h>

#include "esp_netif.h"
#include "driver/mcpwm_cap.h"

// Store handles for capture channels
// Note we set a max of 6 in code
mcpwm_cap_timer_handle_t mcpwm_cap_timer = NULL;
mcpwm_cap_channel_handle_t *mcpwm_cap_channels = NULL;

static bool mcpwm_capture_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data)
{
  PulseInput *pulseInput = static_cast<PulseInput *>(user_data);

  // No point in processing loads of these needlessly
  //if (pulseInput->Count > 100)
  //    return false;

 // pulseInput->Count++;

  if (edata->cap_edge == MCPWM_CAP_EDGE_POS)
  {
    uint32_t currentTime = edata->cap_value;

    // If we have a previous rising edge, calculate the total period
    if (pulseInput->RiseTime != 0)
    {
      pulseInput->TotalPeriodUs = currentTime - pulseInput->RiseTime;

      // Prevent division by zero and filter out noise
      if (pulseInput->TotalPeriodUs > 0 && pulseInput->HighPulseUs > 0 && pulseInput->HighPulseUs <= pulseInput->TotalPeriodUs)
      {
        // Calculate Duty Cycle (0 to 100)
        pulseInput->DutyCycle = (pulseInput->HighPulseUs * 100) / pulseInput->TotalPeriodUs;
        pulseInput->FreshData = true;
      }
    }

    pulseInput->RiseTime = currentTime;
    pulseInput->LastTimestamp = currentTime;
  }
  else if (edata->cap_edge == MCPWM_CAP_EDGE_NEG)
  {
    pulseInput->LastFallTime = edata->cap_value;

    // Falling edge: Calculate how long the signal stayed HIGH during this cycle
    if (pulseInput->RiseTime != 0)
    {
      pulseInput->HighPulseUs = edata->cap_value - pulseInput->RiseTime;
    }
  }

  // if (pulseInput->Count == 90)
  //  ets_printf("Duty cycle: %lu%%\n", pulseInput->DutyCycle);

  return false;
}

void setupPulseInputs()
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

  Serial.println();
  Serial_INFO;
  Serial.println("🎚 Pulse Inputs on MCPWM Capture: " + String(PulseInputs_Count));

  int count = PulseInputs_Count;

  // Enforce the absolute ESP32-S3 hardware limit of 6 capture channels across both units
  if (count > 6)
  {
    Serial_ERROR;
    Serial.println("Warning: ESP32-S3 hardware MCPWM capture max is 6 channels. Limiting to 6.");
    count = 6;
  }

  // Dynamically allocate the exact number of channel handles needed
  if (mcpwm_cap_channels != NULL)
  {
    free(mcpwm_cap_channels);
  }
  mcpwm_cap_channels = (mcpwm_cap_channel_handle_t *)calloc(count, sizeof(mcpwm_cap_channel_handle_t));
  if (mcpwm_cap_channels == NULL)
  {
    Serial_ERROR;
    Serial.println("Failed to allocate memory for MCPWM capture channels!");
    return;
  }

  // 1. Create a shared high-resolution hardware capture timer (1 MHz -> 1 tick = 1 µs)
  mcpwm_capture_timer_config_t cap_timer_config = {};
  cap_timer_config.group_id = 0;
  cap_timer_config.clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT;
  cap_timer_config.resolution_hz = 1000000; // 1 MHz tick rate

  ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_timer_config, &mcpwm_cap_timer));

  // 2. Configure individual input channels for each pulse pin up to 'count'
  for (int i = 0; i < count; i++)
  {
    PulseInput *pulseInput = PulseInputs[i];
    Serial.print("..." + String(pulseInput->Label));

    pinMode(pulseInput->Pin, INPUT_PULLUP);

    mcpwm_capture_channel_config_t cap_chan_config = {};
    cap_chan_config.gpio_num = (gpio_num_t)pulseInput->Pin;
    cap_chan_config.prescale = 1;
    // Capture both positive and negative edges to map full pulse widths
    cap_chan_config.flags.pos_edge = true;
    cap_chan_config.flags.neg_edge = true;
    cap_chan_config.flags.pull_up = true;

    ESP_ERROR_CHECK(mcpwm_new_capture_channel(mcpwm_cap_timer, &cap_chan_config, &mcpwm_cap_channels[i]));

    // Register the hardware interrupt callback for this channel
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = mcpwm_capture_callback,
    };

    // Pass pulseInput directly into user_data
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(mcpwm_cap_channels[i], &cbs, (void *)pulseInput));

    // Enable the capture channel
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(mcpwm_cap_channels[i]));

    Serial.printf(" started on pin %d via MCPWM hardware capture\n", pulseInput->Pin);
  }

  // 3. Start the shared hardware capture timer
  ESP_ERROR_CHECK(mcpwm_capture_timer_enable(mcpwm_cap_timer));
  ESP_ERROR_CHECK(mcpwm_capture_timer_start(mcpwm_cap_timer));

  Serial.println("MCPWM capture initialized successfully");
}
