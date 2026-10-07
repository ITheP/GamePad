#pragma once

#include <Arduino.h>
#include <string>
#include <unordered_map>

struct SerialDebugFunction
{
    std::string Instruction;
    std::string Description;
    std::string Details;
    void (*Function)();
};

class SerialDebug
{
public:
    static void Init();
    static void CheckSerialInput();

private:
    static void ShowHelp();
    static void AddSerialDebugFunctions(SerialDebugFunction serialDebugFunctions[], int count);

    static std::unordered_map<std::string, SerialDebugFunction*> FunctionMap;
    static SerialDebugFunction SerialDebugFunctions[];
    static const int SerialDebugFunctionsCount;
};
