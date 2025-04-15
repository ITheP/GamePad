#pragma once

#include <string>

class Benchmark
{
public:
    // Constructor
    Benchmark();

    void Start(const char* description, int showInt = true);
    void Snapshot(const char* description, int doIt = true);

private:
    unsigned long StartTime;
    unsigned long SnapTime;
};