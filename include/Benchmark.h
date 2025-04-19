#pragma once

#include <string>

class Benchmark
{
public:
    // Constructor
    Benchmark(const char* name);

    void Start(const char* description, int showInt = true);
    void Snapshot(const char* description, int doIt = true);

private:
    const char* Name;
    unsigned long StartTime;
    unsigned long SnapTime;
};