#pragma once

// Critical failure flash codes for LED's
// ...number of times LED flashes
// Make sure numbers have some separation so its easier for people to visually
// count without going `Oh erm was that 4 flashes or 5`
enum WarningFlashCodes
{
  DisplayFailedToStart = 2,
  LittleFSFailedToMount = 4,
  NVSFailed = 6
};

// Low level warning flashing of LED to indicate errors
void WarningFlashes(WarningFlashCodes code);
extern "C" void esp_panic_handler(void* frame);

class Debug {
    public:
        static void WarningFlashes(WarningFlashCodes code);
};