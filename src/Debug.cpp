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
#include "Utils.h"
#include "Version.h"
#include "esp_private/panic_internal.h"
#include <esp_debug_helpers.h>

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
        uint32_t ps;
        uint32_t a1;
        uint32_t a2;
        uint32_t a3;
        uint32_t a4;
        uint32_t a5;
        uint32_t a6;
        uint32_t a7;
        uint32_t a8;
        uint32_t a9;
        uint32_t a10;
        uint32_t a11;
        uint32_t a12;
        uint32_t a13;
        uint32_t a14;
        uint32_t a15;
        uint32_t sar;
        uint32_t exccause;
        uint32_t excvaddr;
        uint32_t lbeg;
        uint32_t lend;
        uint32_t lcount;
        uint32_t BackTrace[32];         // backtrace PCs
    };

    RTC_NOINIT_ATTR PanicRecord PanicInfo;
}

// Print "0x12345678"
static void BackTracePrintHex32(uint32_t v) {
    panic_print_str("0x");
    panic_print_hex(v);
}

// Print one backtrace entry: " 0xPC:0xSP"
static void BackTracePrintEntry(uint32_t pc, uint32_t sp) {
    panic_print_str(" ");
    BackTracePrintHex32(pc);
    panic_print_str(":");
    BackTracePrintHex32(sp);
}

// Print "LABEL : 0x12345678"
static void PanicPrintReg(const char *label, uint32_t value)
{
    panic_print_str(label);
    panic_print_str("0x");
    panic_print_hex(value);
}

// Main walker
extern "C" void IRAM_ATTR PrintBackTrace(const XtExcFrame *frame)
{
    if (!frame) {
        panic_print_str("Backtrace: no frame\n");
        return;
    }

    panic_print_str("Backtrace:");

    // Seed from exception frame, NOT from esp_backtrace_get_start
    esp_backtrace_frame_t bt_frame = {0};
    bt_frame.exc_frame = (void *)frame;
    bt_frame.pc = frame->pc;
    bt_frame.sp = frame->a1;
    bt_frame.next_pc = frame->a0;

    if (!esp_stack_ptr_is_sane(bt_frame.sp) ||
        !esp_ptr_executable((void *)esp_cpu_process_stack_pc(bt_frame.pc)))
    {
        panic_print_str(" <no backtrace>\n");
        return;
    }

    // Print first entry (faulting PC) - use esp_cpu_process_stack_pc to strip windowed ABI bits
    BackTracePrintEntry(esp_cpu_process_stack_pc(bt_frame.pc), bt_frame.sp);

    for (int depth = 1; depth < 32; depth++)
    {
        if (!esp_backtrace_get_next_frame(&bt_frame))
            break;

        BackTracePrintEntry(esp_cpu_process_stack_pc(bt_frame.pc), bt_frame.sp);
    }

    panic_print_char('\n');
}

extern "C" void IRAM_ATTR SaveBackTraceToPanicRecord(PanicRecord *rec, const XtExcFrame *frame)
{
    // Clear the backtrace buffer
    for (int i = 0; i < 32; i++) {
        rec->BackTrace[i] = 0;
    }

    // Seed from exception frame, NOT from esp_backtrace_get_start
    esp_backtrace_frame_t bt_frame = {0};
    bt_frame.exc_frame = (void *)frame;
    bt_frame.pc = frame->pc;
    bt_frame.sp = frame->a1;
    bt_frame.next_pc = frame->a0;

    if (!esp_stack_ptr_is_sane(bt_frame.sp) ||
        !esp_ptr_executable((void *)esp_cpu_process_stack_pc(bt_frame.pc)))
        return;

    // Store processed PC (strip windowed ABI bits)
    rec->BackTrace[0] = esp_cpu_process_stack_pc(bt_frame.pc);

    for (int depth = 1; depth < 32; depth++)
    {
        if (!esp_backtrace_get_next_frame(&bt_frame))
            break;

        rec->BackTrace[depth] = esp_cpu_process_stack_pc(bt_frame.pc);
    }
}

// Our override
// extern "C" void esp_panic_handler(void *info);

// Pointer to original panic handler, so we can it from our override
extern "C" void __real_esp_panic_handler(void *info);

// extern "C" void IRAM_ATTR esp_panic_handler(void *info)
extern "C" void IRAM_ATTR __wrap_esp_panic_handler(void *info)
{
    if (info)
    {
        // Based on esp_panic_handler from C:\Users\<user>.platformio\packages\framework-espidf\components\esp_system\panic.c
        panic_info_t *details = reinterpret_cast<panic_info_t *>(info);

#if defined(__XTENSA__)
        XtExcFrame *frame = (XtExcFrame *)details->frame;

        PanicInfo.flag = PanicFlag;
        PanicInfo.pc = frame->pc;
        PanicInfo.sp = frame->a1; // stack pointer is a1
        PanicInfo.a0 = frame->a0;
        PanicInfo.ps = frame->ps;
        PanicInfo.a1 = frame->a1;
        PanicInfo.a2 = frame->a2;
        PanicInfo.a3 = frame->a3;
        PanicInfo.a4 = frame->a4;
        PanicInfo.a5 = frame->a5;
        PanicInfo.a6 = frame->a6;
        PanicInfo.a7 = frame->a7;
        PanicInfo.a8 = frame->a8;
        PanicInfo.a9 = frame->a9;
        PanicInfo.a10 = frame->a10;
        PanicInfo.a11 = frame->a11;
        PanicInfo.a12 = frame->a12;
        PanicInfo.a13 = frame->a13;
        PanicInfo.a14 = frame->a14;
        PanicInfo.a15 = frame->a15;
        PanicInfo.sar = frame->sar;
        PanicInfo.exccause = frame->exccause;
        PanicInfo.excvaddr = frame->excvaddr;
        PanicInfo.lbeg = frame->lbeg;
        PanicInfo.lend = frame->lend;
        PanicInfo.lcount = frame->lcount;

        SaveBackTraceToPanicRecord(&PanicInfo, frame);
#endif

        if (details->reason)
        {
            panic_print_str("\nBuild: ");
            panic_print_str(GetBuildVersion());
            panic_print_str("\nCustom handler...\n");
            panic_print_str("Guru Meditation Error: Core ");
            panic_print_dec(details->core);
            panic_print_str(" panic'ed (");
            panic_print_str(details->reason);
            panic_print_str("). ");
        }

        if (details->description)
        {
            panic_print_str(details->description);
        }

        panic_print_str("\n\nCore ");
        panic_print_dec(details->core);
        panic_print_str(" register dump:\n");

        PanicPrintReg("PC      : ", frame->pc);
        panic_print_str("  ");
        PanicPrintReg("PS      : ", frame->ps);
        panic_print_str("  ");
        PanicPrintReg("A0      : ", frame->a0);
        panic_print_str("  ");
        PanicPrintReg("A1      : ", frame->a1);
        panic_print_char('\n');

        PanicPrintReg("A2      : ", frame->a2);
        panic_print_str("  ");
        PanicPrintReg("A3      : ", frame->a3);
        panic_print_str("  ");
        PanicPrintReg("A4      : ", frame->a4);
        panic_print_str("  ");
        PanicPrintReg("A5      : ", frame->a5);
        panic_print_char('\n');

        PanicPrintReg("A6      : ", frame->a6);
        panic_print_str("  ");
        PanicPrintReg("A7      : ", frame->a7);
        panic_print_str("  ");
        PanicPrintReg("A8      : ", frame->a8);
        panic_print_str("  ");
        PanicPrintReg("A9      : ", frame->a9);
        panic_print_char('\n');

        PanicPrintReg("A10     : ", frame->a10);
        panic_print_str("  ");
        PanicPrintReg("A11     : ", frame->a11);
        panic_print_str("  ");
        PanicPrintReg("A12     : ", frame->a12);
        panic_print_str("  ");
        PanicPrintReg("A13     : ", frame->a13);
        panic_print_char('\n');

        PanicPrintReg("A14     : ", frame->a14);
        panic_print_str("  ");
        PanicPrintReg("A15     : ", frame->a15);
        panic_print_str("  ");
        PanicPrintReg("SAR     : ", frame->sar);
        panic_print_str("  ");
        PanicPrintReg("EXCCAUSE: ", frame->exccause);
        panic_print_char('\n');

        PanicPrintReg("EXCVADDR: ", frame->excvaddr);
        panic_print_str("  ");
        PanicPrintReg("LBEG    : ", frame->lbeg);
        panic_print_str("  ");
        PanicPrintReg("LEND    : ", frame->lend);
        panic_print_str("  ");
        PanicPrintReg("LCOUNT  : ", frame->lcount);
        panic_print_char('\n');

        PrintBackTrace(frame);
    }

    panic_print_str("\n\nInitiating system panic handler. Should match custom...\n");

    // Call the original handler
    // Theoretically should reboot for us too
    __real_esp_panic_handler(info);

    // Decide what to do next
    while (true)
    {
    }
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
        file.println("Build: " + String(GetBuildVersion()));
        file.println("Reset Reason: " + String(ResetReasonToString(reason)) + " (" + String((int)reason) + ")\n");

        if (PanicInfo.flag == PanicFlag)
        {
            file.println("PanicInfo:");
            file.printf("  pc=0x%08lx sp=0x%08lx a0=0x%08lx\n", PanicInfo.pc, PanicInfo.sp, PanicInfo.a0);
            file.printf("  exccause=%lu excvaddr=0x%08lx\n\n", PanicInfo.exccause, PanicInfo.excvaddr);

            file.printf("Core register dump:\n");
            file.printf("PC      : 0x%08lx  PS      : 0x%08lx  A0      : 0x%08lx  A1      : 0x%08lx\n",
                        PanicInfo.pc, PanicInfo.ps, PanicInfo.a0, PanicInfo.a1);
            file.printf("A2      : 0x%08lx  A3      : 0x%08lx  A4      : 0x%08lx  A5      : 0x%08lx\n",
                        PanicInfo.a2, PanicInfo.a3, PanicInfo.a4, PanicInfo.a5);
            file.printf("A6      : 0x%08lx  A7      : 0x%08lx  A8      : 0x%08lx  A9      : 0x%08lx\n",
                        PanicInfo.a6, PanicInfo.a7, PanicInfo.a8, PanicInfo.a9);
            file.printf("A10     : 0x%08lx  A11     : 0x%08lx  A12     : 0x%08lx  A13     : 0x%08lx\n",
                        PanicInfo.a10, PanicInfo.a11, PanicInfo.a12, PanicInfo.a13);
            file.printf("A14     : 0x%08lx  A15     : 0x%08lx  SAR     : 0x%08lx  EXCCAUSE: 0x%08lx\n",
                        PanicInfo.a14, PanicInfo.a15, PanicInfo.sar, PanicInfo.exccause);
            file.printf("EXCVADDR: 0x%08lx  LBEG    : 0x%08lx  LEND    : 0x%08lx  LCOUNT  : 0x%08lx\n\n",
                        PanicInfo.excvaddr, PanicInfo.lbeg, PanicInfo.lend, PanicInfo.lcount);

            // Note the saved backtrace values only have the PC (Program Counter) values, not the SP S(Stack Pointer). They aren't needed for post-mortem analysis.
            file.print("Backtrace:");
            for (size_t i = 0; i < 32; i++)
            {
                uint32_t pc = PanicInfo.BackTrace[i];
                if (pc == 0)
                    break;
                file.printf(" 0x%08lx", static_cast<unsigned long>(pc));
            }
            file.println("\n");
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

    Serial.println("Build: " + String(GetBuildVersion()));
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

        Serial.println("Core register dump:");
        Serial.printf("PC      : 0x%08lx  PS      : 0x%08lx  A0      : 0x%08lx  A1      : 0x%08lx\n",
                      PanicInfo.pc, PanicInfo.ps, PanicInfo.a0, PanicInfo.a1);
        Serial.printf("A2      : 0x%08lx  A3      : 0x%08lx  A4      : 0x%08lx  A5      : 0x%08lx\n",
                      PanicInfo.a2, PanicInfo.a3, PanicInfo.a4, PanicInfo.a5);
        Serial.printf("A6      : 0x%08lx  A7      : 0x%08lx  A8      : 0x%08lx  A9      : 0x%08lx\n",
                      PanicInfo.a6, PanicInfo.a7, PanicInfo.a8, PanicInfo.a9);
        Serial.printf("A10     : 0x%08lx  A11     : 0x%08lx  A12     : 0x%08lx  A13     : 0x%08lx\n",
                      PanicInfo.a10, PanicInfo.a11, PanicInfo.a12, PanicInfo.a13);
        Serial.printf("A14     : 0x%08lx  A15     : 0x%08lx  SAR     : 0x%08lx  EXCCAUSE: 0x%08lx\n",
                      PanicInfo.a14, PanicInfo.a15, PanicInfo.sar, PanicInfo.exccause);
        Serial.printf("EXCVADDR: 0x%08lx  LBEG    : 0x%08lx  LEND    : 0x%08lx  LCOUNT  : 0x%08lx\n\n",
                      PanicInfo.excvaddr, PanicInfo.lbeg, PanicInfo.lend, PanicInfo.lcount);

        Serial.print("Backtrace:");
        for (size_t i = 0; i < 32; i++)
        {
            uint32_t pc = PanicInfo.BackTrace[i];
            if (pc == 0)
                break;
            Serial.printf(" 0x%08lx", static_cast<unsigned long>(pc));
        }
        Serial.println("\n");
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