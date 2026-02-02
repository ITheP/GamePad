#pragma once

// Default WiFi and Password values, used if no value found in profile
// though for security, stick your own WiFi and password values in secrets.h which shouldn't get published into the git repo
// Recommend it is left blank to force user to set up their own WiFi details
// A missing password is allowed (for open networks) but a missing SSID is not allowed as this would be insecure
#define WIFI_DEFAULT_SSID ""
#define WIFI_DEFAULT_PASSWORD ""

// If secrets.h exists, include it
#if __has_include("secrets.h")
#include "secrets.h"
#endif
