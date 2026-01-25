#include "Version.h"
#include <cstring>
#include <cstdio>
#include <GamePad.h>

const char *GetBuildVersion()
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
