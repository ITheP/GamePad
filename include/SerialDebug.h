#pragma once

#include <Arduino.h>
#include <string>
#include <unordered_map>

enum class SerialDebugFunctionParameterType : uint8_t
{
    None = 0,
    Int,
    Float,
    Bool,
    String
};

// ------------------------------------------------------------
// Trampolines
//
// Each parameterised command stores a pointer to one of these. The
// compiler generates a distinct instance per <T, Fn> pair, so the type
// of the target handler is encoded in the pointer's identity — no
// runtime cast, no UB.
// ------------------------------------------------------------

// Handler takes T by value: void Fn(T)
template <typename T, void (*Fn)(T)>
static void SerialDebugTrampoline(void *value)
{
    Fn(*static_cast<T *>(value));
}


struct SerialDebugFunction
{
    std::string Instruction;
    std::string Description;
    std::string Details;

    // No-parameter command — set when ParameterType == None
    void (*Function)();

    // Parameterised command — set when ParameterType != None
    SerialDebugFunctionParameterType ParameterType = SerialDebugFunctionParameterType::None;
    void (*ParameterFunction)(void *) = nullptr;

    bool HasParameter() const
    {
        return ParameterType != SerialDebugFunctionParameterType::None;
    }

};

class SerialDebug
{
public:
    static void Init();
    static void CheckSerialInput();
    static void ShowHelp();

    static void AddSerialDebugFunctions(SerialDebugFunction serialDebugFunctions[], int count);

    // Command tables supplied by other modules
    static SerialDebugFunction SerialDebugFunctions[];
    static const int SerialDebugFunctionsCount;

    // Lookup
    static std::unordered_map<std::string, SerialDebugFunction *> FunctionMap;

private:
    // Ordered list of pointers, used by ShowHelp so groups appear in
    // registration order.
    static std::vector<SerialDebugFunction *> FunctionOrder;
};