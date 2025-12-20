#pragma once

#include <Arduino.h>
#include "config.h"

// ANSI escape codes
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_RESET   "\033[0m"

// Toggle colours: define USE_COLOURS before including this header
#ifdef SERIAL_USE_COLOURS
  #define Serial_OK      Serial.print(ANSI_GREEN  "[OK  ] " ANSI_RESET)
  #define Serial_ERROR   Serial.print(ANSI_RED    "[ERR ] " ANSI_RESET)
  #define Serial_WARN    Serial.print(ANSI_YELLOW "[WARN] " ANSI_RESET)
  #define Serial_INFO    Serial.print(ANSI_CYAN   "[INFO] " ANSI_RESET)
#else
  #define Serial_OK      Serial.print("[OK  ] ")
  #define Serial_ERROR   Serial.print("[ERR ] ")
  #define Serial_WARN    Serial.print("[WARN] ")
  #define Serial_INFO    Serial.print("[INFO] ")
#endif



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
extern "C" void panic_handler(void *frame);

class Debug
{
public:
  static char CrashFile[];
  static RTC_NOINIT_ATTR char CrashInfo[1024];

  static void Mark(int mark);
  static void RecordMark();

  static void PowerOnInit();
  static void WarningFlashes(WarningFlashCodes code);
  static void CheckForCrashInfo();
  static void CrashOnPurpose();
};