#include <Arduino.h>
#include <Benchmark.h>
#include <string>

Benchmark::Benchmark(const char *name) : Name(name) {};

void Benchmark::Start(const char *description, int showIt)
{
    StartTime = micros();
    SnapTime = StartTime;

    if (showIt)
        Serial.printf("⏱ %s: Start @ %s\n",
                      Name,
                      description);
}

void Benchmark::Snapshot(const char *description, int showIt)
{
    // Reminder: microseconds = milliseconds * 0.001;
    unsigned long now = micros();
    unsigned long totalTimeTaken = (now - StartTime);
    unsigned long timeTaken = (now - SnapTime);

    if (showIt)
        Serial.printf(
            "⏱ %s: %.2fms / %.2fms %s\n",
            Name,
            timeTaken * 0.001,
            totalTimeTaken * 0.001,
            description);

    SnapTime = now;
}