#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_cpu.h>
#include <xtensa/core-macros.h>
#include "Debug.h"
#include "config.h"
#include "Prefs.h"

// Low level warning flashing of LED to indicate errors
void Debug::WarningFlashes(WarningFlashCodes code)
{
    while (1)
    {
        // Forever flash internal LED to let us know theres something wrong!
        for (int i = 0; i < (int)code; i++)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(150);
            digitalWrite(LED_BUILTIN, LOW);
            delay(150);
        }
        delay(750);
    }
}

const char* decodeCause(uint32_t cause) {
    switch (cause) {
        case 0: return "Illegal Instruction";
        case 3: return "LoadProhibited";
        case 6: return "StoreProhibited";
        case 9: return "DivideByZero";
        default: return "Unknown";
    }
}

extern "C" void panic_handler(void *frame)
{
    XtExcFrame *exc = (XtExcFrame *)frame;
    uint32_t cause = exc->exccause;
    uint32_t pc = exc->pc;

    LittleFS.begin(true);
    File file = LittleFS.open("/crash.log", FILE_WRITE);
    if (file)
    {
        file.println("<h1>Low-Level Crash Detected</h1>");
        file.println("Boot Count: <b>" + String(Prefs::BootCount) + "</b></br>");
        file.println("Cause Code: <b>" + String(cause) + "</b></br>");
        file.print("Cause Description: <b>");
        file.print(decodeCause(cause));
        file.println("</b></br>");
        file.println("Program Counter (PC): <b>0x" + String(pc, HEX) + "</b></br>");
        file.println("Timestamp: <b>" + String(millis()) + "</b></br>");
        file.println("------------------------");
        file.close();
    }

    esp_restart(); // while(true);
}
