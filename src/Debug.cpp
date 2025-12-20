#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_cpu.h>
#include <xtensa/core-macros.h>
#include "Debug.h"
#include "config.h"
#include "Prefs.h"

// Possible extra debug assistance on crashes
// https://kotyara12.ru/iot/remote_esp32_backtrace/


char Debug::CrashFile[] = "/debug/crash.log";
RTC_NOINIT_ATTR char Debug::CrashInfo[1024];
RTC_NOINIT_ATTR int MarkVal;

void Debug::Mark(int mark)
{
    MarkVal = mark;
}

void Debug::RecordMark()
{
    snprintf(CrashInfo + strlen(CrashInfo), sizeof(CrashInfo) - strlen(CrashInfo), ", %d", MarkVal);
}

void Debug::PowerOnInit()
{
    memset(CrashInfo, 0, sizeof(CrashInfo));
    sprintf(CrashInfo, "Marks: Init");
}

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

// Experiments in device crash logging...
// DOESN'T WORK without changing framework from arduino to espidf (or layering in arduino inside espidf)
// but that in itself suddenly opens a whole can of worms!
// Stopped bothering

// extern "C" void panic_handler(void *frame)
// {
//     //Serial.println("PANIC HANDLER - CRITICAL PROBLEM OCCURED");
//     sprintf(CrashInfo, "PANIC IN THE DISCO");

//     return;

//     XtExcFrame *exc = (XtExcFrame *)frame;
//     uint32_t cause = exc->exccause;
//     uint32_t pc = exc->pc;

//     const char* causeDescription;
//             digitalWrite(LED_BUILTIN, HIGH);
//             delay(500);
//             digitalWrite(LED_BUILTIN, LOW);

//     switch (cause) {
//     case 0:
//         causeDescription = "Illegal Instruction";
//         break;
//     case 3:
//         causeDescription = "LoadProhibited";
//         break;
//     case 6:
//         causeDescription = "StoreProhibited";
//         break;
//     case 9:
//         causeDescription = "DivideByZero";
//         break;
//     default:
//         causeDescription = "Unknown";
//         break;
//     }

//    // char buf[256];

//     snprintf(CrashInfo, sizeof(CrashInfo),
//             "Boot Count: <b>%d</b></br>\n"
//             "Cause Code: <b>%u</b></br>\n"
//             "Cause Description: <b>%s</b></br>\n"
//             "Program Counter (PC): <b>0x%08X</b></br>\n",
//             Prefs::BootCount,
//             cause,
//             causeDescription,
//             pc);

//     Serial.println("Low-Level Crash Detected<");
//     Serial.println("Boot Count: " + String(Prefs::BootCount));
//     Serial.println("Cause Code: " + String(cause));
//     Serial.println("Cause Description: "  + String(causeDescription));
//     Serial.println("Program Counter (PC): 0x" + String(pc, HEX));
//     Serial.println("Timestamp: " + String(millis()));

//     //LittleFS.begin(true);
//     File file = LittleFS.open("/debug/test.log", FILE_WRITE);
//     if (file)
//     {
//         file.println("<h1>Low-Level Crash Detected</h1>");
//         file.println("Boot Count: <b>" + String(Prefs::BootCount) + "</b></br>");
//         file.println("Cause Code: <b>" + String(cause) + "</b></br>");
//         file.println("Cause Description: <b>" + String(causeDescription) + "</b></br>");
//         file.println("Program Counter (PC): <b>0x" + String(pc, HEX) + "</b></br>");
//         file.println("Timestamp: <b>" + String(millis()) + "</b></br>");
//         file.close();
//     }

//     Serial.println("Restarting in 5 seconds");
//     delay(5000);

//     while(true)
//     {
//                     digitalWrite(LED_BUILTIN, HIGH);
//                     delay(100);
//                                 digitalWrite(LED_BUILTIN, LOW);
//                                 delay(100);
//     }

//     esp_restart(); // while(true);
// }

void Debug::CheckForCrashInfo()
{
    RecordMark();

    if (CrashInfo[0] == 0)
        Serial.println("No crash info found");
    return;

    Serial.println("Crash info found, logging to " + String(CrashFile) + " - resulting file should also be visible in web interface");

    // Crash file picked up! Save this
    File file = LittleFS.open(CrashFile, FILE_WRITE);
    if (file)
    {
        file.print(CrashInfo);
        file.close();
    }

    Serial.print(CrashInfo);

    CrashInfo[0] = 0;
}

void Debug::CrashOnPurpose()
{
    Serial.println("ATTEMPTING TO CRASH DEVICE");

    Serial.println("Access invalid memory");
    int *ptr = nullptr;
    int crash = *ptr; // 💥 LoadProhibited panic

    Serial.println("Div by zero");
    int x = 10; // Assume x = 0
    x += -x;
    int y = 100 / x; // 💥 Only if x == 0 at runtime

    Serial.println("Access Invalid Memory");
    volatile int *bad = (int *)0xDEADBEEF;
    crash = *bad;
}