#include "Utils.h"
#include <cstring>
#include <cstdio>
#include <GamePad.h>
#include <LittleFS.h>

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

const char *getBuildVersion()
{
  static char version[32]; // Enough space for "v123.YYMMDD.HHMMSS"
  static bool initialized = false;

  if (initialized)
    return version;

  // Parse __DATE__ → "Oct 16 2025"
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char monthStr[4];
  int month = 0, day = 0, year = 0;

  sscanf(__DATE__, "%3s %d %d", monthStr, &day, &year);

  // Convert month string to MM
  for (int i = 0; i < 12; ++i)
  {
    if (strncmp(monthStr, months + i * 3, 3) == 0)
    {
      month = i + 1;
      break;
    }
  }

  // __TIME__ → "13:53:42"
  int hour = 0, minute = 0, second = 0;
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

  // Format: v1YYMMDD.HHMMSS
  snprintf(version, sizeof(version), "v%s.%02d%02d%02d.%02d%02d", COREVERSION, year % 100, month, day, hour, minute);

  return version;
}

void DumpFileToSerial(const char* path) {
if (LittleFS.exists(path))
{

  File file = LittleFS.open(path, FILE_READ);
  if (!file) {
    Serial.printf("ℹ️ Failed to open %s\n", path);   // ❌
    return;
  }

  Serial.printf("📄 Dumping %s (%d bytes):\n", path, file.size());

  while (file.available()) {
    Serial.write(file.read());
  }

  file.close();
}
else
  Serial.printf("ℹ️ %s doesn't exist to dump to serial", path);
}

// SaferPasswordString only shows first and last letters - all others are converted to * - safter for display anywhere (including Serial use)
String SaferPasswordString(String password)
{
    int len = password.length();

    // If 0, 1, or 2 chars → return as‑is
    if (len <= 2)
        return password;

    // Build masked version
    String result = "";
    result += password[0];          // first char
    for (int i = 1; i < len - 1; i++)
        result += '*';              // mask middle
    result += password[len - 1];    // last char

    return result;
}

String SaferShortenedPasswordString(String password)
{
    int len = password.length();

    // If 0, 1, or 2 chars → return as‑is
    if (len <= 2)
        return password;

    // Build masked version
    String result = "";
    result += password[0];          // first char
    result += "...";                // make up middle
    result += password[len - 1];    // last char

    return result;
}