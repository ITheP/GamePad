#pragma once

#include <stdint.h>

//#define EXTRA_SERIAL_DEBUG        // Enable to print loads of extra information to serial
//#define EXTRA_SERIAL_DEBUG_PLUS   // ...further detail, but can flood serial output somewhat!
//#define INCLUDE_BENCHMARKS        // Includes some basic performance statistics of running device over serial
//#define INCLUDE_BENCHMARKS_LED    // Basic performance stats for LED processing
//#define WHITE_SCREEN              // Display will show a solid white screen, handy when physically aligning panel in device

#define SETUP_DELAY     250         // Delay between stages of initial start up
                                    // Could be pretty much instant, but having a delay gives a chance to display
                                    // startup information on screen when we turn the device on,
                                    // and have a chance to actually see it. And looks nice :)

#define DEBOUNCE_DELAY  10000       // Button debounce delay (10000 == 10ms)
                                    // Some info on timings - https://www.digikey.com/en/articles/how-to-implement-hardware-debounce-for-switches-and-relays?form=MG0AV3

// XBox 360 X-Plorer Guitar Hero Controller (model 95065)
// HID/XInput variant of BLE Gamepad https://github.com/Mystfit/ESP32-BLE-CompositeHID
// USB\VID_1430&PID_4748 and Compatible Id of USB\MS_COMP_XUSB10 (to tell the driver that it's a wired xbox 360 controller).
#define VID 1430
#define PID 4748

#define SERIAL_SPEED 921600             // 115200 also common
#define SERIAL_USE_COLOURS              // Enable coloured output in serial monitor

#define SUB_SECOND_COUNT         30     // Number of sub-samples to take across a second when calculating clicks per second counts. Higher number = more accurate but more overhead.
#define DISPLAY_UPDATE_RATE      0.016  // Fraction of a second to wait between display updates - 0.016 = 16ms = ~60FPS
#define LED_UPDATE_RATE          0.005  // Fraction of a second to wait between throttled updates - default is faster than 60fps to account for animation effects where we want faster propagation across LED arrays

// Some standard values
#define LOW 0x0
#define HIGH 0x1

#define ON 0x0
#define OFF 0x1

#define PRESSED 0x0
#define NOT_PRESSED 0x1
#define LONG_PRESS_MONITORING 0x2
#define LONG_PRESS 0x3

enum ControllerReport {
    ReportToController = 0,
    DontReportToController = 1
};

#define NONE 0

// class Config {
//     public:
//         static void Init();

//         static uint32_t BootCount;
// };