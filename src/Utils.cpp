#include "Utils.h"
#include <cstring>
#include <cstdio>
#include <GamePad.h>
#include <LittleFS.h>
#include <Debug.h>

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void DumpFileToSerial(const char* path) {
if (LittleFS.exists(path))
{
  File file = LittleFS.open(path, FILE_READ);
  if (!file) {
    Serial_INFO;
    Serial.printf("❌ Failed to open %s\n", path);
    return;
  }

  Serial_INFO;
  Serial.printf("📄 Showing %s (%d bytes):\n", path, file.size());

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
    result += "....";                // make up middle
    result += password[len - 1];    // last char

    return result;
}