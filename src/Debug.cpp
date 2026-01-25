#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_cpu.h>
#include <xtensa/core-macros.h>
#include <climits>
#include <algorithm>
#include <vector>
#if defined(__XTENSA__)
#include <xtensa/xtensa_context.h>
#endif
#include "Debug.h"
#include "Config.h"
#include "Defines.h"
#include "Prefs.h"
#include "esp_private/panic_internal.h"

// Override the default panic handler
// We have to do this in C linkage to match the original declaration
// You can check for original panic handler name equivalent in .elf file in e.g. powershell...
// & "$env:USERPROFILE\.platformio\packages\toolchain-xtensa-esp32s3\bin\xtensa-esp32s3-elf-nm.exe" -n .pio/build/esp32-s3-devkitm-1/firmware.elf | Select-String "esp_panic_handler"
// assuming using esp32-s3 toolchain here, and esp_panic_handler is correct

// Example core dump from system...
//
// Guru Meditation Error: Core  0 panic'ed (LoadProhibited).
// Exception was unhandled.

// Core 0 register dump:
// PC      : 0x42012345  PS      : 0x00060030  A0      : 0x4200abcd
// A1      : 0x3fcdf1a0  A2      : 0x00000000  A3      : 0x3fcdf1b0
// A4      : 0x00000004  A5      : 0x3fcdf1c0  A6      : 0x00000001
// A7      : 0x00000000  A8      : 0x800d1234  A9      : 0x3fcdf180
// A10     : 0x00000000  A11     : 0x3fcdf1d0  A12     : 0x00000000
// A13     : 0x00000001  A14     : 0x00000000  A15     : 0x3fcdf1e0

// Backtrace: 0x42012345:0x3fcdf1a0 0x4200abcd:0x3fcdf1c0 0x42004567:0x3fcdf1e0

// ELF file SHA256: 1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcd

// Rebooting...

// Our override
// extern "C" void esp_panic_handler(void *info);

// Pointer to original panic handler, so we can it from our override
// extern "C" void esp_panic_handler_original(void *info)
//     __attribute__((weak, alias("esp_panic_handler")));

extern "C" void __real_esp_panic_handler(void *info);

//extern "C" void IRAM_ATTR esp_panic_handler(void *info)
extern "C" void IRAM_ATTR __wrap_esp_panic_handler(void *info)
{

    panic_info_t *details = reinterpret_cast<panic_info_t *>(info);

    // Your logging here — but keep it IRAM‑safe
    panic_print_str("\n\n--- CUSTOM PANIC HANDLER TRIGGERED ---\n");
    //ets_printf("CUSTOM PANIC TRIGGERED\n");

    if (details->reason) {
        panic_print_str("## Guru Meditation Error: Core ");
        panic_print_dec(details->core);
        panic_print_str(" panic'ed (");
        panic_print_str(details->reason);
        panic_print_str(").\n");
    }

    if (details->description) {
        panic_print_str("## Description:\n");
        panic_print_str(details->description);
    }

        panic_print_str("\n## CUSTOM COMPLETE - HANDING OVER TO ORIGINAL:\n");

    // Call the original handler
    // Theoretically should reboot for us too
    __real_esp_panic_handler(info);


    // Decide what to do next
    while (true)
    {
    }
}

// extern "C" void __real_esp_panic_handler(void *info);

// extern "C" void IRAM_ATTR __wrap_esp_panic_handler(void *info)
// {
// #if defined(__XTENSA__)
//     if (info)
//     {
//         XtExcFrame *frame = reinterpret_cast<XtExcFrame *>(info);
//         PanicInfo.flag = PanicFlag;
//         PanicInfo.pc = frame->pc;
//         PanicInfo.sp = frame->a1; // stack pointer is a1
//         PanicInfo.a0 = frame->a0;
//         PanicInfo.exccause = frame->exccause;
//         PanicInfo.excvaddr = frame->excvaddr;
//     }
// #endif
//     __real_esp_panic_handler(info);
// }

char Debug::CrashFile[] = "/debug/crash.log";
const char Debug::CrashDir[] = "/debug";

namespace
{
    constexpr size_t DebugMarkCount = 24;
}

RTC_NOINIT_ATTR Debug::DebugMark DebugMarks[DebugMarkCount];
RTC_NOINIT_ATTR int DebugMarkPos;

namespace
{
    constexpr uint32_t PanicFlag = 0x4F554348; // "OUCH"
    constexpr char CrashPrefix[] = "crash.";
    constexpr char CrashSuffix[] = ".log";

    struct PanicRecord
    {
        uint32_t flag;
        uint32_t pc;
        uint32_t sp;
        uint32_t a0;
        uint32_t exccause;
        uint32_t excvaddr;
    };

    RTC_NOINIT_ATTR PanicRecord PanicInfo;
}

static void ClearDebugMark(Debug::DebugMark &mark)
{
    mark.Value = -1;
    mark.LineNumber = 0;
    mark.Filename[0] = '\0';
    mark.Function[0] = '\0';
    mark.CrashInfo[0] = '\0';
}

static void SetMarkString(char *dest, size_t destSize, const char *src)
{
    if (!dest || destSize == 0)
        return;

    if (src && src[0] != '\0')
    {
        strncpy(dest, src, destSize - 1);
        dest[destSize - 1] = '\0';
    }
    else
    {
        dest[0] = '\0';
    }
}

static void StoreDebugMark(int value, int lineNumber, const char *filename, const char *functionName, const char *details)
{
    if (DebugMarkPos < 0 || DebugMarkPos >= static_cast<int>(DebugMarkCount))
        DebugMarkPos = 0;

    DebugMarkPos = (DebugMarkPos + 1) % static_cast<int>(DebugMarkCount);

    Debug::DebugMark &entry = DebugMarks[DebugMarkPos];
    entry.Value = value;
    entry.LineNumber = lineNumber > 0 ? lineNumber : 0;
    SetMarkString(entry.Filename, sizeof(entry.Filename), filename);
    SetMarkString(entry.Function, sizeof(entry.Function), functionName);
    SetMarkString(entry.CrashInfo, sizeof(entry.CrashInfo), details);
}

static bool ParseCrashIndex(const char *name, uint32_t &index)
{
    if (!name)
        return false;

    const char *base = strrchr(name, '/');
    base = base ? base + 1 : name;

    size_t baseLen = strlen(base);
    size_t prefixLen = strlen(CrashPrefix);
    size_t suffixLen = strlen(CrashSuffix);

    if (baseLen <= prefixLen + suffixLen)
        return false;

    if (strncmp(base, CrashPrefix, prefixLen) != 0)
        return false;

    if (strcmp(base + baseLen - suffixLen, CrashSuffix) != 0)
        return false;

    const char *numStart = base + prefixLen;
    size_t numLen = baseLen - prefixLen - suffixLen;
    if (numLen == 0)
        return false;

    uint32_t value = 0;
    for (size_t i = 0; i < numLen; i++)
    {
        char c = numStart[i];
        if (c < '0' || c > '9')
            return false;
        value = value * 10 + static_cast<uint32_t>(c - '0');
    }

    index = value;
    return true;
}

const char *Debug::GetLatestCrashFilePath()
{
    static char latestPath[32];

    File dir = LittleFS.open(CrashDir);
    if (!dir || !dir.isDirectory())
        return nullptr;

    uint32_t maxIndex = 0;
    bool found = false;

    File file = dir.openNextFile();
    while (file)
    {
        uint32_t index = 0;
        if (ParseCrashIndex(file.name(), index))
        {
            if (!found || index > maxIndex)
            {
                maxIndex = index;
                found = true;
            }
        }
        file = dir.openNextFile();
    }

    if (!found)
        return nullptr;

    snprintf(latestPath, sizeof(latestPath), "%s/crash.%05lu.log", CrashDir, static_cast<unsigned long>(maxIndex));
    return latestPath;
}

void Debug::GetCrashLogPaths(std::vector<String> &outPaths, bool newestFirst)
{
    outPaths.clear();

    File dir = LittleFS.open(CrashDir);
    if (!dir || !dir.isDirectory())
        return;

    struct CrashItem
    {
        uint32_t index;
        String path;
    };

    std::vector<CrashItem> items;

    File file = dir.openNextFile();
    while (file)
    {
        uint32_t index = 0;
        if (ParseCrashIndex(file.name(), index))
        {
            String name = String(file.name());
            String path;

            // Just incase the file name has a full path from some unforseen changes to the FS processing used
            String crashPrefix = String(CrashDir) + "/";
            if (name.startsWith(crashPrefix))
                path = name;
            else
                path = crashPrefix + name;

            items.push_back({index, path});
        }
        file = dir.openNextFile();
    }

    std::sort(items.begin(), items.end(), [newestFirst](const CrashItem &a, const CrashItem &b)
              { return newestFirst ? (a.index > b.index) : (a.index < b.index); });

    for (const auto &item : items)
        outPaths.push_back(item.path);
}

// Crash files survive firmware re-flashes - will only blank to nothing
// when filesystem image is re-written
// Crash files are written on a rolling basis, keeping up to 10 most recent (to save space)
// If we reach the max reference number (99999) we wipe all existing crash files and start again from 0
bool Debug::GetNextCrashFilePath(char *outPath, size_t outPathSize)
{
    if (!outPath || outPathSize == 0)
        return false;

    // Return default file if crash folder doesn't exist
    File dir = LittleFS.open(CrashDir);
    if (!dir || !dir.isDirectory())
    {
        if (!LittleFS.mkdir(CrashDir))
            return false;

        // Default filename
        snprintf(outPath, outPathSize, "%s/crash.%05lu.log", CrashDir, 1UL);
        return true;
    }

    uint32_t minIndex = UINT_MAX;
    uint32_t maxIndex = 0;
    uint8_t count = 0;

    File file = dir.openNextFile();
    while (file)
    {
        uint32_t index = 0;
        if (ParseCrashIndex(file.name(), index))
        {
            count++;
            if (index < minIndex)
                minIndex = index;
            if (index > maxIndex)
                maxIndex = index;
        }
        file = dir.openNextFile();
    }

    /// 99999 is a lot of crashes, but you never know over the years!
    if (maxIndex >= 99999)
    {
        File wipeDir = LittleFS.open(CrashDir);
        File wipeFile = wipeDir.openNextFile();
        while (wipeFile)
        {
            uint32_t index = 0;
            // Check if file is a crash file before deleting
            if (ParseCrashIndex(wipeFile.name(), index))
            {
                LittleFS.remove(wipeFile.name());
            }
            wipeFile = wipeDir.openNextFile();
        }

        // Return default crash file, starting from 0 so we have a reference that it's a fresh start
        snprintf(outPath, outPathSize, "%s/crash.%05lu.log", CrashDir, 0UL);
        return true;
    }

    // If there are 10 files already, get rid of the first to keep down storage usage on device
    if (count >= 10 && minIndex != UINT_MAX)
    {
        char removePath[32];
        snprintf(removePath, sizeof(removePath), "%s/crash.%05lu.log", CrashDir, static_cast<unsigned long>(minIndex));
        LittleFS.remove(removePath);
    }

    uint32_t nextIndex = (maxIndex > 0) ? (maxIndex + 1) : 1;
    snprintf(outPath, outPathSize, "%s/crash.%05lu.log", CrashDir, static_cast<unsigned long>(nextIndex));
    return true;
}

void Debug::Mark(int mark)
{
    StoreDebugMark(mark, 0, nullptr, nullptr, nullptr);
}

void Debug::Mark(int mark, const char *details)
{
    StoreDebugMark(mark, 0, nullptr, nullptr, details);
}

void Debug::Mark(int mark, int lineNumber, const char *filename, const char *function)
{
    StoreDebugMark(mark, lineNumber, filename, function, nullptr);
}

void Debug::Mark(int mark, int lineNumber, const char *filename, const char *function, const char *details)
{
    StoreDebugMark(mark, lineNumber, filename, function, details);
}

// Reset any possible crash info - we've just powered on so there won't be any!
void Debug::ClearCrashCheckData()
{
    for (size_t i = 0; i < DebugMarkCount; i++)
        ClearDebugMark(DebugMarks[i]);

    DebugMarkPos = 0;
    PanicInfo.flag = 0;
}

static const char *ResetReasonToString(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_POWERON:
        return "Power on";
    case ESP_RST_EXT:
        return "External reset";
    case ESP_RST_SW:
        return "Software reset";
    case ESP_RST_PANIC:
        return "Panic";
    case ESP_RST_INT_WDT:
        return "Interrupt watchdog";
    case ESP_RST_TASK_WDT:
        return "Task watchdog";
    case ESP_RST_WDT:
        return "Other watchdog";
    case ESP_RST_DEEPSLEEP:
        return "Deep sleep";
    case ESP_RST_BROWNOUT:
        return "Brownout";
    case ESP_RST_SDIO:
        return "SDIO";
    default:
        return "Unknown";
    }
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

// Checks for any logging/crash info recorded prior to last device reset and logs it to a file if we find anything
void Debug::CheckForCrashInfo(esp_reset_reason_t reason)
{
    // We don't check for marks unless some sort of relevant panic flag was set

    if (reason == ESP_RST_POWERON ||
        reason == ESP_RST_EXT ||
        reason == ESP_RST_SW)
    {
        // Valid reset reasons rather than from issues. Ignore.
        return;
    }

    bool hasMarks = (DebugMarks[0].Value == -1 ? false : true);

    // if (!hasMarks && PanicInfo.flag != PanicFlag)
    // {
    //     Serial.println("No crash info found");
    //     return;
    // }

    char crashPath[32];
    if (!GetNextCrashFilePath(crashPath, sizeof(crashPath)))
    {
        Serial.print("💥 ⛔ Critical failure, unable to create crash file. No crash details will be saved.");
        return;
    }

    Serial.println("💥 Crash detected, logging to " + String(crashPath) + " - resulting file should also be visible in web interface");

    // Crash file picked up! Save this
    File file = LittleFS.open(crashPath, FILE_WRITE);
    if (file)
    {
        file.println("Reset Reason: " + String(ResetReasonToString(reason)) + " (" + String((int)reason) + ")\n");

        if (PanicInfo.flag == PanicFlag)
        {
            file.println("PanicInfo:");
            file.printf("  pc=0x%08lx sp=0x%08lx a0=0x%08lx\n", PanicInfo.pc, PanicInfo.sp, PanicInfo.a0);
            file.printf("  exccause=%lu excvaddr=0x%08lx\n\n", PanicInfo.exccause, PanicInfo.excvaddr);
        }

        if (hasMarks)
        {

            file.println("Marks (newest first):");

            for (size_t i = 0; i < DebugMarkCount; i++)
            {
                int idx = DebugMarkPos - static_cast<int>(i);
                if (idx < 0)
                    idx += static_cast<int>(DebugMarkCount);

                const Debug::DebugMark &mark = DebugMarks[idx];
                if (mark.Value == -1)
                    continue;

                // e.g.
                // [ 300] src/GamePad.cpp.MainLoop().73  : Hat Inputs
                // [ 240] src/GamePad.cpp.MainLoop().1486: Something Else
                // [ 240] ?.?.773 : Unknown Something
                // [  17] Other details
                // [1372]

                // Always print the value
                file.printf("[%4d]", mark.Value);

                if (mark.LineNumber > 0)
                {
                    const char *fileName = (mark.Filename[0] != '\0') ? mark.Filename : "?";
                    const char *functionName = (mark.Function[0] != '\0') ? mark.Function : "?";
                    int lineNumber = (mark.LineNumber > 0) ? mark.LineNumber : 0;

                    if (mark.CrashInfo[0] == '\0')
                        file.printf(" %s.%s().%d\n", fileName, functionName, lineNumber);
                    else
                        file.printf(" %s.%s().%-4d: %s\n", fileName, functionName, lineNumber, mark.CrashInfo);
                }
                else if (mark.CrashInfo[0] != '\0')
                    file.printf(" %s\n", mark.CrashInfo);
                else
                    file.println();
            }
        }
        file.close();
    }

    Serial.print("ResetReason: ");
    Serial.print(ResetReasonToString(reason));
    Serial.print(" (");
    Serial.print((int)reason);
    Serial.println(")");
    Serial.println();

    if (PanicInfo.flag == PanicFlag)
    {
        Serial.printf("PanicInfo: pc=0x%08lx sp=0x%08lx a0=0x%08lx\n", PanicInfo.pc, PanicInfo.sp, PanicInfo.a0);
        Serial.printf("           exccause=%lu excvaddr=0x%08lx\n", PanicInfo.exccause, PanicInfo.excvaddr);
        Serial.println();
    }

    if (hasMarks)
    {
        Serial.println("Marks (newest first):");

        // Limit to 3 most recent for serial output, avoids flooding console
        for (size_t i = 0; i < DebugMarkCount; i++)
        {
            int idx = DebugMarkPos - static_cast<int>(i);
            if (idx < 0)
                idx += static_cast<int>(DebugMarkCount);

            const Debug::DebugMark &mark = DebugMarks[idx];
            if (mark.Value == -1)
                continue;

            // Always print the value
            Serial.printf("[%4d]", mark.Value);

            if (mark.LineNumber > 0)
            {
                const char *fileName = (mark.Filename[0] != '\0') ? mark.Filename : "?";
                const char *functionName = (mark.Function[0] != '\0') ? mark.Function : "?";
                int lineNumber = (mark.LineNumber > 0) ? mark.LineNumber : 0;

                if (mark.CrashInfo[0] == '\0')
                    Serial.printf(" %s.%s().%d\n", fileName, functionName, lineNumber);
                else
                    Serial.printf(" %s.%s().%-4d: %s\n", fileName, functionName, lineNumber, mark.CrashInfo);
            }
            else if (mark.CrashInfo[0] != '\0')
                Serial.printf(" %s\n", mark.CrashInfo);
            else
                Serial.println();
        }

        Serial.println();
    }
}

// Primarily for testing purposes
void Debug::CrashOnPurpose()
{
    Serial.println("💥 ATTEMPTING TO CRASH DEVICE");

    Serial.println("💥 Access invalid memory");
    int *ptr = nullptr;
    int crash = *ptr; // 💥 LoadProhibited panic

    Serial.println("Div by zero");
    int x = 10; // Assume x = 0
    x += -x;
    int y = 100 / x; // 💥 Only if x == 0 at runtime

    Serial.println("💥 Access Invalid Memory");
    volatile int *bad = (int *)0xDEADBEEF;
    crash = *bad;
}