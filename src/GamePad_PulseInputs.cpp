#include "GamePad_PulseInputs.h"
#include <Structs.h>
#include <Debug.h>
#include <DeviceConfig.h>

// #include "esp_netif.h"
// // #5 #include "driver/mcpwm_cap.h"
// #include "driver/mcpwm.h"
// #include "driver/pcnt.h"
// #include "esp_attr.h"

// #include "driver/rmt.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "driver/gpio.h"
#include <soc/soc.h>

#define MAX_MCPWM 3 // ESP-IDF v4 supports CAP0, CAP1, CAP2 (3 capture inputs per unit)
// Global instance - MUST be in DRAM
//static PulseInput g_pulseInput;

// Simple ISR - ONLY uses IRAM-safe functions
static void IRAM_ATTR capture_isr(void *user_data)
{
  // Cast user_data to PulseInput pointer
  PulseInput *pulse = (PulseInput *)user_data;

  // Get the current capture value - IRAM safe
  uint32_t current_value = mcpwm_capture_signal_get_value(
      pulse->MCPWM_Unit,
      pulse->MCPWM_CaptureSignal);

  // Check if this is a new capture
  if (current_value == pulse->LastCaptureValue)
  {
    return;
  }

  // Read GPIO level - IRAM safe
  int gpio_level = gpio_get_level((gpio_num_t)pulse->Pin);

  // Process the edge
  if (gpio_level == 1)
  { // Rising edge
    pulse->LastRisingEdge = current_value;

    if (pulse->LastCaptureValue > 0 && pulse->LastWasHigh)
    {
      pulse->PeriodTicks = current_value - pulse->LastCaptureValue;
      pulse->HasValidPeriod = true;
    }

    if (pulse->LastFallingEdge > 0)
    {
      pulse->HighTicks = current_value - pulse->LastFallingEdge;
    }

    pulse->LastWasHigh = true;
  }
  else
  { // Falling edge
    pulse->LastFallingEdge = current_value;

    if (pulse->LastRisingEdge > 0)
    {
      pulse->HighTicks = current_value - pulse->LastRisingEdge;
    }

    pulse->LastWasHigh = false;
  }

  pulse->LastCaptureValue = current_value;

  // Calculate duty cycle and frequency
  if (pulse->HasValidPeriod && pulse->PeriodTicks > 0)
  {
    // Use integer arithmetic first
    uint32_t duty_x1000 = (pulse->HighTicks * 1000) / pulse->PeriodTicks;
    pulse->DutyCycle = (float)duty_x1000 / 10.0f; // Convert to percentage with 1 decimal
    pulse->Frequency = 1000000 / pulse->PeriodTicks;
  }
}

volatile int TestBob;

volatile uint32_t s_captured_period = 0;
volatile uint32_t s_captured_high = 0;
volatile uint32_t s_last_timestamp = 0;
volatile bool s_is_high = false;

static bool IRAM_ATTR test4(
    mcpwm_unit_t mcpwm, 
    mcpwm_capture_channel_id_t cap_channel, 
    const cap_event_data_t *edata, 
    void *user_data)
    {
      
// edata->cap_value provides the hardware timestamp directly from the capture register
    uint32_t current = edata->cap_value; 
    uint32_t delta = current - s_last_timestamp;
    s_last_timestamp = current;

    if (s_is_high) {
        s_captured_high = delta;
    } else {
        s_captured_period = s_captured_high + delta;
    }
    
    // Toggle state based on edge direction present in edata
    s_is_high = (edata->cap_edge == MCPWM_POS_EDGE);

    TestBob++;
    // pulseInput->Count++;
    return false;
}

static void IRAM_ATTR capture_isr_test1(void *user_data) {
    PulseInput *pulse = (PulseInput *)user_data;
    
    // Just read the capture value - this is safe
    uint32_t current_value = mcpwm_capture_signal_get_value(
        pulse->MCPWM_Unit, 
        pulse->MCPWM_CaptureSignal
    );
    TestBob++;
    // Store it
    pulse->LastCaptureValue = current_value;
}


void setupPulseInputs() {
    Serial.println();
    Serial.println("🎚 Pulse Input via Hardware MCPWM - Count: " + String(PulseInputs_Count));

    if (PulseInputs_Count < 1) {
        Serial.println("❌ No pulse inputs configured!");
        return;
    }
esp_err_t err;
    PulseInput *sourceInput = PulseInputs[0];
    int pin = sourceInput->Pin;

    sourceInput->Pin = pin;
    sourceInput->LastCaptureValue = 0;
    sourceInput->PeriodTicks = 0;
    sourceInput->HighTicks = 0;
    sourceInput->LastWasHigh = false;
    sourceInput->LastRisingEdge = 0;
    sourceInput->LastFallingEdge = 0;
    sourceInput->HasValidPeriod = false;
    sourceInput->Frequency = 0;
    sourceInput->MCPWM_Unit = MCPWM_UNIT_0;
    sourceInput->MCPWM_CaptureSignal = MCPWM_SELECT_CAP0;

    // 1. Configure GPIO
    gpio_reset_pin((gpio_num_t)pin);
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
    gpio_pullup_en((gpio_num_t)pin);

    mcpwm_unit_t unit = MCPWM_UNIT_0;
    mcpwm_timer_t timer = MCPWM_TIMER_0;

    // 2. Initialize MCPWM timer
    mcpwm_config_t timer_config = {
        .frequency = 1000000,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .duty_mode = MCPWM_DUTY_MODE_0,
        .counter_mode = MCPWM_UP_COUNTER,
    };
    err = mcpwm_init(unit, timer, &timer_config);
Serial.printf("Setup C %d\n", err);

    // 3. Route GPIO to the capture pin
    err = mcpwm_gpio_init(unit, MCPWM_CAP_0, pin);
    Serial.printf("Setup D %d\n", err);


//     // 4. IMPORTANT: Register ISR BEFORE enabling capture to avoid crashes [citation:11]
//    err = mcpwm_isr_register(
//         unit,
//         test_isr, //capture_isr,
//         NULL, //(void *)&g_pulseInput,
//         ESP_INTR_FLAG_IRAM, // Use IRAM flag for speed
//         NULL
//     );
// Serial.printf("Setup E %d\n", err);

//     if (err != ESP_OK) {
//         Serial.printf("❌ ISR registration failed: %s\n", esp_err_to_name(err));
//         return;
//     }

// 5. For ESP-IDF v4.4 on ESP32-S3, use the new API with configuration structure
    // The old mcpwm_capture_enable() is deprecated and may cause issues
    mcpwm_capture_config_t capture_config = {
        .cap_edge = MCPWM_BOTH_EDGE,  // Capture on both edges
        .cap_prescale = 0,            // No prescale (0 means 1)
        .capture_cb = test4,           // No callback (using ISR instead)
        .user_data = NULL //(void *)sourceInput
    };

    err = mcpwm_capture_enable_channel(unit, MCPWM_SELECT_CAP0, &capture_config);

    Serial.printf("Setup F - mcpwm_capture_enable_channel: %d\n", err);

    if (err != ESP_OK) {
        Serial.printf("❌ Capture enable failed: %s\n", esp_err_to_name(err));
        return;
    }

    Serial.printf("✅ Hardware MCPWM Capture ready on pin %d\n", pin);

    delay(5000);
}


// void setupPulseInputs()
// {
//       esp_err_t err;

//   Serial.println();
//   Serial.println("🎚 Pulse Input(s) via Legacy MCPWM (IDF v4) - Count: " + String(PulseInputs_Count));

//   if (PulseInputs_Count < 1)
//   {
//     Serial.println("❌ No pulse inputs configured!");
//     delay(10000);
//     return;
//   }

//   // Copy configuration from the external array to our global instance
//   PulseInput *sourceInput = PulseInputs[0];
//   int pin = sourceInput->Pin;

//   // Initialize the global instance with values from the source
//   sourceInput->Pin = pin;
//   sourceInput->LastCaptureValue = 0;
//   sourceInput->PeriodTicks = 0;
//   sourceInput->HighTicks = 0;
//   sourceInput->LastWasHigh = false;
//   sourceInput->LastRisingEdge = 0;
//   sourceInput->LastFallingEdge = 0;
//   sourceInput->HasValidPeriod = false;
//   sourceInput->DutyCycle = 0.0f;
//   sourceInput->Frequency = 0;
//   sourceInput->MCPWM_Unit = MCPWM_UNIT_0;
//   sourceInput->MCPWM_CaptureSignal = MCPWM_SELECT_CAP0;
// Serial.println("Setup A"); delay(2500);
// //esp_err_t err;

//   // Configure GPIO
//   err = gpio_reset_pin((gpio_num_t)pin);
// Serial.printf("Setup A %d\n", err);
//   gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);

// Serial.printf("Setup B %d\n", err);
//   gpio_pullup_en((gpio_num_t)pin);

// Serial.printf("Setup C %d\n", err);
//   mcpwm_unit_t unit = MCPWM_UNIT_0;
//   mcpwm_timer_t timer = MCPWM_TIMER_0;

//   // Initialize MCPWM timer (1 MHz -> 1 tick = 1 µs)
//   mcpwm_config_t timer_config = {
//       .frequency = 1000000,
//       .cmpr_a = 0,
//       .cmpr_b = 0,
//       .duty_mode = MCPWM_DUTY_MODE_0,
//       .counter_mode = MCPWM_UP_COUNTER,
//   };

//   err = mcpwm_init(unit, timer, &timer_config);

// Serial.printf("Setup D %d\n", err);

//   // Route GPIO to CAP0 pin
//   err = mcpwm_gpio_init(unit, MCPWM_CAP_0, pin);

// Serial.printf("Setup E %d\n", err); delay(2500);

// // Enable capture for both edges
//   err = mcpwm_capture_enable(unit, MCPWM_SELECT_CAP0, MCPWM_BOTH_EDGE, 0);

// Serial.printf("Setup F %d\n", err); delay(2500);

//     err = mcpwm_isr_register(
//         MCPWM_UNIT_0,
//         test_isr,
//         NULL,
//         0,
//         NULL
//     );
    
//     Serial.printf("test_isr registration: %d\n", err);
//     delay(10000);
//     return;


// // Register ISR - pass the global instance
//   err = mcpwm_isr_register(
//       unit,
//       test_isr,
//       (void *)&g_pulseInput, // Pass the global instance
//       0,
//       NULL);

// Serial.printf("Setup G %d\n", err); delay(2500);
//   if (err != ESP_OK)
//   {
//     Serial.printf("❌ Failed to register ISR: %d\n", err);
//     delay(10000);
//   }
//   else
//   {
//     Serial.printf("✅ Legacy MCPWM Capture ready on pin %d\n", pin);
//     delay(10000);
//   }
// }

// // 2 units available
// #define MAX_MCPWM 2

// // ISR – runs on every edge
// static void IRAM_ATTR capture_isr(void *user_data)
// {
//   PulseInput *pulseInput = static_cast<PulseInput *>(user_data);

//     uint32_t current = mcpwm_capture_signal_get_value(pulseInput->MCPWM_Unit, pulseInput->MCPWM_CaptureSignal);
//     uint32_t delta   = current - pulseInput->LastCaptureValue;
//     pulseInput->LastCaptureValue   = current;

//     // MCPWM capture gives the timer value; we need the edge direction
//     // On ESP32-S3 the capture ISR is triggered on both edges when configured that way.
//     // We determine direction from the GPIO level (fast enough at 20 kHz).
//     bool level = gpio_get_level((gpio_num_t)pulseInput->Pin);

//     if (pulseInput->LastWasHigh) {
//         // previous period was high → this edge is falling
//         pulseInput->HighTicks = delta;
//     } else {
//         // previous period was low → this edge is rising → full period complete
//         pulseInput->PeriodTicks = pulseInput->HighTicks + delta;
//     }
//     pulseInput->LastWasHigh = level;
// }

// void setupPulseInputs()
// {
// #ifdef DEBUG_MARKS
//   Debug::Mark(1, __LINE__, __FILE__, __func__);
// #endif

//   Serial.println();
//   Serial_INFO;
//   Serial.println("🎚 Pulse Input(s) via MCPWM - Count: " + String(PulseInputs_Count));

//   // Determine how many pins we can configure based on available RMT channels
//   int channelsToConfig = (PulseInputs_Count < MAX_MCPWM) ? PulseInputs_Count : MAX_MCPWM;

//   if (PulseInputs_Count > MAX_MCPWM)
//   {
//     Serial.printf("⚠️ WARNING: You have %d pulse inputs, but ESP32-S3 legacy RMT only supports %d channels. Only the first %d will be configured.\n",
//                   PulseInputs_Count, MAX_MCPWM, MAX_MCPWM);
//   }

//   for (int i = 0; i < channelsToConfig; i++)
//   {

//     PulseInput *pulseInput = PulseInputs[i];

//     // GPIO
//     auto pin = pulseInput->Pin;
//     gpio_reset_pin((gpio_num_t)pin);
//     gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
//     gpio_pullup_en((gpio_num_t)pin);          // or pulldown according to your signal

//     mcpwm_unit_t unit = (mcpwm_unit_t)i;
//     mcpwm_timer_t timer = (mcpwm_timer_t)i;
//     mcpwm_capture_signal_t captureSignal = (mcpwm_capture_signal_t)i;

//     pulseInput->LastCaptureValue = 0;
//     pulseInput->PeriodTicks   = 0;
//     pulseInput->HighTicks     = 0;
//     pulseInput->LastWasHigh  = false;

//     pulseInput->MCPWM_Unit = unit;
//     pulseInput->MCPWM_CaptureSignal = captureSignal;

//     // MCPWM timer – free-running, highest resolution
//     mcpwm_config_t timer_config = {
//         .frequency    = 1000000,          // 1 MHz → 1 tick = 1 µs (easy maths)
//         .cmpr_a       = 0,
//         .cmpr_b       = 0,
//         .duty_mode    = MCPWM_DUTY_MODE_0,
//         .counter_mode = MCPWM_UP_COUNTER,
//     };
//     mcpwm_init(unit, timer, &timer_config);

//     // Capture on both edges
//     mcpwm_capture_enable(unit, captureSignal, MCPWM_BOTH_EDGE, 0);

//     // Route the GPIO to the capture input
//     mcpwm_io_signals_t captureSignalIo = (i == 0) ? MCPWM_CAP_0 : (i == 1) ? MCPWM_CAP_1 : MCPWM_CAP_2;
//     mcpwm_gpio_init(unit, captureSignalIo, pin);   // CAP0

//     // Install ISR
//     mcpwm_isr_register(unit, capture_isr, (void *)pulseInput, ESP_INTR_FLAG_IRAM, NULL);

//     Serial.println("MCPWM Capture ready");
//   }
// }

// void updateMcpwmPulse()
// {
//     // Take a snapshot (ISR can update the volatiles at any time)
//     uint32_t period = period_ticks;
//     uint32_t high   = high_ticks;

//     if (period == 0) return;          // no edges yet

//     float frequency = 1000000.0f / period;          // Hz (because timer = 1 MHz)
//     float duty      = (100.0f * high) / period;     // %

//     // Optional – simple low-pass / average
//     static float filtFreq = 0, filtDuty = 0;
//     filtFreq = filtFreq * 0.8f + frequency * 0.2f;
//     filtDuty = filtDuty * 0.8f + duty * 0.2f;

//     Serial.printf("Freq: %.1f Hz   Duty: %.1f %%\n", filtFreq, filtDuty);

//     // Store into your PulseInput structure if you want
//     // pulseInput->TotalPeriodUs = period;
//     // pulseInput->HighPulseUs   = high;
//     // pulseInput->DutyCycle     = (int)duty;
//     // pulseInput->FreshData     = true;
// }

// #define RMT_RX_CHANNEL RMT_CHANNEL_0
// #define RMT_CLK_DIV 80      // Assuming 80MHz APB clock -> 1 tick = 1 microsecond (1us resolution!)
// #define RMT_TICK_10_US (80) // Just a scale reference if needed

// #define MAX_RMT_CHANNELS 4
// #define RMT_RX_CHANNEL_OFFSET 4 // ESP32-S3 legacy RX channels are 4 through 7

// // #define RMT_DEFAULT_CONFIG_RX(gpio, channel_id) \
// //     {                                           \
// //         .rmt_mode = RMT_MODE_RX,                \
// //         .channel = channel_id,                  \
// //         .gpio_num = gpio,                       \
// //         .clk_div = 80,                          \
// //         .mem_block_num = 1,                     \
// //         .flags = 0,                             \
// //         .rx_config = {                          \
// //             .idle_threshold = 12000,            \
// //             .filter_ticks_thresh = 100,         \
// //             .filter_en = true,                  \
// //         }                                       \
// //     }

// void setupPulseInputs()
// {
// #ifdef DEBUG_MARKS
//   Debug::Mark(1, __LINE__, __FILE__, __func__);
// #endif

//   Serial.println();
//   Serial_INFO;
//   Serial.println("🎚 Pulse Input(s) via Legacy RMT RX - Count: " + String(PulseInputs_Count));

//   // Determine how many pins we can configure based on available RMT channels
//   int channelsToConfig = (PulseInputs_Count < MAX_RMT_CHANNELS) ? PulseInputs_Count : MAX_RMT_CHANNELS;

//   if (PulseInputs_Count > MAX_RMT_CHANNELS)
//   {
//     Serial.printf("⚠️ WARNING: You have %d pulse inputs, but ESP32-S3 legacy RMT only supports %d channels. Only the first %d will be configured.\n",
//                   PulseInputs_Count, MAX_RMT_CHANNELS, MAX_RMT_CHANNELS);
//   }

//   for (int i = 0; i < channelsToConfig; i++)
//   {

//     PulseInput *pulseInput = PulseInputs[i];
//     if (!pulseInput)
//       continue;

//     rmt_channel_t rmtChan = (rmt_channel_t)(RMT_RX_CHANNEL_OFFSET + i);

//     // Just in case, reset pin mode
//     pinMode(pulseInput->Pin, INPUT);

//     Serial.printf("... %s:", pulseInput->Label);

//     rmt_config_t config;
//     // rmt_config_t rmt_rx_config = RMT_DEFAULT_CONFIG_RX((gpio_num_t)pulseInput->Pin, 5);
//     config.rmt_mode = RMT_MODE_RX;
//     config.channel = rmtChan;
//     config.gpio_num = (gpio_num_t)pulseInput->Pin;
//     config.clk_div = 80;      // 80MHz APB clock / 80 = 1 tick per microsecond (1us resolution)
//     config.mem_block_num = 3; // Use 1 memory block (64 items) per channel
//     config.flags = 0;
//     config.rx_config.filter_en = true;
//     config.rx_config.filter_ticks_thresh = 1;  // Filter out noise spikes
//     config.rx_config.idle_threshold = 40;    // 0x80;    // 10ms of inactivity marks end of stream

//     pulseInput->RMTChannel = rmtChan;

//     // Initialize and install driver for this specific channel
//     esp_err_t err = rmt_config(&config);
//     if (err != ESP_OK)
//     {
//       Serial.printf(", [ERROR] RMT config failed for pin %d: 0x%X\n", pulseInput->Pin, err);
//       continue;
//     }
//     else
//     {
//       Serial.printf(", RMT config OK (returned 0x%X)", err);
//     }

//     //4096 * sizeof(rmt_item32_t)
//     err = rmt_driver_install(rmtChan, 2048*3, ESP_INTR_FLAG_SHARED);
//     if (err != ESP_OK)
//     {
//       Serial.printf(", [ERROR] RMT driver install failed for pin %d: 0x%X\n", pulseInput->Pin, err);
//       continue;
//     }
//     else
//     {
//       Serial.printf(", RMT Driver Install OK (returned 0x%X)", err);
//     }

//     err = rmt_rx_start(rmtChan, true); // Start receiving immediately with ring buffer enabled
//     if (err != ESP_OK)
//     {
//       Serial.printf(", [ERROR] RMT rx start failed for pin %d: 0x%X", pulseInput->Pin, err);
//     }
//     else
//     {
//       Serial.printf(", RMT RX %d channel %d started on pin %d", i, rmtChan, pulseInput->Pin);
//     }

//     Serial.println();
//   }

//   Serial.println("Legacy RMT pulse setup complete");
// }

// void updatePulseInputs()
// {
//   int channelsToRead = (PulseInputs_Count < MAX_RMT_CHANNELS) ? PulseInputs_Count : MAX_RMT_CHANNELS;

//   for (int i = 0; i < channelsToRead; i++)
//   {
//     PulseInput *pulseInput = PulseInputs[i];
//     if (!pulseInput)
//       continue;

//     rmt_channel_t rmtChan = pulseInput->RMTChannel;
//     RingbufHandle_t rb = NULL;
//     rmt_get_ringbuf_handle(rmtChan, &rb);
//     if (!rb) {
//       pulseInput->A = 2;
//       continue;
//     }
//     else
//     {
//       pulseInput->A = 1;
//     }

//     size_t rx_size = 0;
//     rmt_item32_t *item = (rmt_item32_t *)xRingbufferReceive(rb, &rx_size, 0); // Non-blocking check

//     pulseInput->G = rx_size;

//     if (item && rx_size > 0)
//     {
//       pulseInput->F = 1;
//       size_t num_items = rx_size / sizeof(rmt_item32_t);
//       uint32_t totalHighUs = 0;
//       uint32_t totalLowUs = 0;
//       int pulseCount = 0;

//       pulseInput->B = num_items;

//       for (int j = 0; j < num_items; j++)
//       {
//         if (item[j].duration0 == 0 && item[j].duration1 == 0)
//           break;

//         if (item[j].level0 == 1)
//         {
//           totalHighUs += item[j].duration0;
//           totalLowUs += item[j].duration1;
//         }
//         else
//         {
//           totalLowUs += item[j].duration0;
//           totalHighUs += item[j].duration1;
//         }
//         pulseCount++;
//       }

//       vRingbufferReturnItem(rb, (void *)item);

//       uint32_t totalPeriodUs = totalHighUs + totalLowUs;
//       if (totalPeriodUs > 0 && pulseCount > 0)
//       {
//         pulseInput->HighPulseUs = totalHighUs / pulseCount;
//         pulseInput->TotalPeriodUs = totalPeriodUs / pulseCount;
//         pulseInput->DutyCycle = (int)((totalHighUs * 100) / totalPeriodUs);
//         pulseInput->FreshData = true;

//       pulseInput->C = pulseInput->HighPulseUs;
//       pulseInput->D = pulseInput->TotalPeriodUs;
//       pulseInput->E = pulseInput->DutyCycle;

//       }
//     }
//     else
//     {
//       pulseInput->F = 2;
//     }
//     pulseInput->Count++;
//   }
// }

// // #include "esp_netif.h"
// // #include "driver/mcpwm.h"
// // #include "esp_attr.h"

// // // Hardware-triggered ISR callback for ESP-IDF v4.x legacy MCPWM capture
// // static bool IRAM_ATTR mcpwm_capture_default_callback(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_sig, const cap_event_data_t *edata, void *user_data)
// // {
// //   ets_printf("INTERUPT ");

// //   PulseInput *pulseInput = static_cast<PulseInput *>(user_data);
// //   if (!pulseInput) return false;

// //   uint32_t currentTime = edata->cap_value;

// //     ets_printf("%12d ", currentTime);

// //   // Handle Rising Edge
// //   if (edata->cap_edge == MCPWM_POS_EDGE)
// //   {
// //     ets_printf("+ ");
// //     // If we have a previous rising edge timestamp, calculate the total wave period
// //     if (pulseInput->RiseTime != 0)
// //     {
// //       pulseInput->TotalPeriodUs = currentTime - pulseInput->RiseTime;

// //       // Prevent division by zero and filter out invalid/noise periods
// //       if (pulseInput->TotalPeriodUs > 0 && pulseInput->HighPulseUs > 0 && pulseInput->HighPulseUs <= pulseInput->TotalPeriodUs)
// //       {
// //         // Calculate Duty Cycle percentage (0 to 100)
// //         pulseInput->DutyCycle = (pulseInput->HighPulseUs * 100) / pulseInput->TotalPeriodUs;
// //         pulseInput->FreshData = true; // Flag that fresh values are ready to read
// //         ets_printf("Duty Cycle: %4d  ", pulseInput->DutyCycle);
// //       }
// //     }

// //     pulseInput->RiseTime = currentTime;
// //     pulseInput->LastTimestamp = currentTime;
// //   }
// //   // Handle Falling Edge
// //   else if (edata->cap_edge == MCPWM_NEG_EDGE)
// //   {
// //     ets_printf("- ");
// //     pulseInput->LastFallTime = currentTime;

// //     // Calculate how long the signal stayed HIGH during this pulse cycle
// //     if (pulseInput->RiseTime != 0)
// //     {
// //       pulseInput->HighPulseUs = currentTime - pulseInput->RiseTime;
// //     }
// //   }
// // ets_printf(" - done\n");
// //   return false;
// // }

// // void setupPulseInputs()
// // {
// // #ifdef DEBUG_MARKS
// //   Debug::Mark(1, __LINE__, __FILE__, __func__);
// // #endif

// // Serial.println();
// //   Serial_INFO;
// //   Serial.println("🎚 Pulse Inputs via Legacy Hardware MCPWM Capture: " + String(PulseInputs_Count));

// //   int count = PulseInputs_Count;
// //   // // ESP32 legacy hardware capture supports a maximum of 3 channels per unit (CAP0, CAP1, CAP2)
// //   // if (count > 3)
// //   // {
// //   //   Serial_ERROR;
// //   //   Serial.println("Warning: ESP32 legacy hardware capture supports max 3 channels per unit. Limiting to 3.");
// //   //   count = 3;
// //   // }

// //   for (int i = 0; i < count; i++)
// //   {
// //     PulseInput *pulseInput = PulseInputs[i];
// //     //if (!pulseInput) continue;

// //     Serial.print("..." + String(pulseInput->Label));
// //     pinMode(pulseInput->Pin, INPUT_PULLDOWN);
// //     //analogSetPinAttenuation(pulseInput->Pin, ADC_2_5db);
// //   }

// //   // // Configure MCPWM unit 0 capture channels in hardware
// //   // for (int i = 0; i < count; i++)
// //   // {
// //   //   PulseInput *pulseInput = PulseInputs[i];
// //   //   if (!pulseInput) continue;

// //   //   Serial.print("..." + String(pulseInput->Label));
// //   //   pinMode(pulseInput->Pin, INPUT_PULLUP);

// //   //   // 1. Map pin to legacy MCPWM capture GPIO signal
// //   //   mcpwm_io_signals_t cap_signal_io = (i == 0) ? MCPWM_CAP_0 : (i == 1) ? MCPWM_CAP_1 : MCPWM_CAP_2;
// //   //   esp_err_t err = mcpwm_gpio_init(MCPWM_UNIT_0, cap_signal_io, pulseInput->Pin);
// //   //   if (err != ESP_OK)
// //   //   {
// //   //     Serial_ERROR;
// //   //     Serial.printf(" [Failed to init GPIO %d for MCPWM cap signal: 0x%X]\n", pulseInput->Pin, err);
// //   //     continue;
// //   //   }

// //   //   // 2. Select capture channel index selector
// //   //   mcpwm_capture_signal_t cap_signal_sel = (i == 0) ? MCPWM_SELECT_CAP0 : (i == 1) ? MCPWM_SELECT_CAP1 : MCPWM_SELECT_CAP2;

// //   //   // 3. Enable hardware capture on both edges
// //   //   err = mcpwm_capture_enable(MCPWM_UNIT_0, cap_signal_sel, MCPWM_BOTH_EDGE, 0);
// //   //   if (err != ESP_OK)
// //   //   {
// //   //     Serial_ERROR;
// //   //     Serial.printf(" [Failed to enable capture signal selection: 0x%X]\n", err);
// //   //     continue;
// //   //   }

// //   //   // 4. Bind configuration and callback structure for this hardware channel
// //   //   mcpwm_capture_config_t cap_config = {
// //   //       .cap_edge = MCPWM_BOTH_EDGE,
// //   //       .cap_prescale = 1,
// //   //       .capture_cb = mcpwm_capture_default_callback,
// //   //       .user_data = (void *)pulseInput
// //   //   };

// //   //   err = mcpwm_capture_enable_channel(MCPWM_UNIT_0, (mcpwm_capture_channel_id_t)i, &cap_config);
// //   //   if (err != ESP_OK)
// //   //   {
// //   //     Serial_ERROR;
// //   //     Serial.printf(" [Failed to enable capture channel %d: 0x%X]\n", i, err);
// //   //   }
// //   //   else
// //   //   {
// //   //     Serial.printf(" hardware duty-cycle capture started on pin %d\n", pulseInput->Pin);
// //   //   }
// //   // }

// //   Serial.println("Hardware MCPWM capture setup complete");
// // }

// // void updateDutyCycles()
// // {
// //   int count = PulseInputs_Count;
// //   for (int i = 0; i < count; i++) {
// //     PulseInput *pulseInput = PulseInputs[i];
// //     measureDutyCycle(pulseInput);

// //     Serial.printf("Pulse Input TEST %d: %d -> %4d\n", i, pulseInput->Pin, pulseInput->DutyCycle);
// //   }
// // }

// // int measureDutyCycle(PulseInput *pulseInput)
// // {
// //   //if (!pulseInput) return 0;

// //   uint8_t pin = pulseInput->Pin;

// //   auto fred = digitalRead(pin); // analogRead(pin);
// //  Serial.printf("T0 %d  ", fred);
// //   return 9;

// //   uint32_t timeoutUs = 500000; // 50ms timeout

// //   // Cache GPIO register/mask if on ESP32 for ultra-fast reading,
// //   // or use standard digitalRead if simplicity is preferred. Standard is shown here:
// //   uint32_t startTime = micros();

// //   // 1. Wait for signal to go LOW (sync to start of cycle)
// //   while (digitalRead(pin) == HIGH)
// //   {
// //     if ((micros() - startTime) > timeoutUs) { Serial.print("T1 "); return 1;}
// //   }

// //   // 2. Wait for RISING edge (Start of HIGH pulse)
// //   while (digitalRead(pin) == LOW)
// //   {
// //     if ((micros() - startTime) > timeoutUs) { Serial.print("T2 "); return 2;}
// //   }
// //   uint32_t riseTime = micros();

// //   // 3. Wait for FALLING edge (End of HIGH pulse)
// //   while (digitalRead(pin) == HIGH)
// //   {
// //     if ((micros() - startTime) > timeoutUs) { Serial.print("T3 "); return 3;}
// //   }
// //   uint32_t fallTime = micros();

// //   // 4. Wait for next RISING edge to complete the full period
// //   while (digitalRead(pin) == LOW)
// //   {
// //     if ((micros() - startTime) > timeoutUs) { Serial.print("T4 "); return 4;}
// //   }
// //   uint32_t nextRiseTime = micros();

// //   // 5. Calculate metrics
// //   uint32_t highPulseUs = fallTime - riseTime;
// //   uint32_t totalPeriodUs = nextRiseTime - riseTime;

// //   if (totalPeriodUs > 0 && highPulseUs <= totalPeriodUs)
// //   {
// //     // OPTIMIZATION 2: Store values into struct while we have them
// //     pulseInput->HighPulseUs = highPulseUs;
// //     pulseInput->TotalPeriodUs = totalPeriodUs;

// //     int dutyCycle = (int)((highPulseUs * 100) / totalPeriodUs);
// //     pulseInput->DutyCycle = dutyCycle;
// //     pulseInput->FreshData = true;

// //     return dutyCycle;
// //   }
// //  Serial.print("T5 ");
// //   return 0;
// // }