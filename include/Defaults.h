#pragma once

// Default WiFi and Password values, used if no value found in profile
// though for security, stick your own WiFi and password values in secrets.h which shouldn't get published into the git repo
#define WIFI_DEFAULT_SSID ""
#define WIFI_DEFAULT_PASSWORD ""

// If secrets.h exists, include it
#if __has_include("secrets.h")
#include "secrets.h"
#endif
