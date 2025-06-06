#pragma once

#include <stdint.h>
#include "esp_wifi.h"

class Network {
public:
    static void HandleWifi(int second);
};