#include <Arduino.h>
#include <Benchmark.h>
#include <string>

Benchmark::Benchmark() {};

void Benchmark::Start(const char* description, int showIt)
{
    StartTime = micros();
    SnapTime = StartTime;

    if (showIt)
        Serial.println("PERFORMANCE: Start @ " + String(description));
}

void Benchmark::Snapshot(const char *description, int showIt)
{
    // Reminder: microseconds = milliseconds * 0.001;
    unsigned long now = micros();
    unsigned long totalTimeTaken = (now - StartTime);
    unsigned long timeTaken = (now - SnapTime);

    if (showIt)
        Serial.println("PERFORMANCE: " + String(timeTaken * 0.001, 2) + "ms / " + String(totalTimeTaken * 0.001, 2) + "ms " + String(description));

    SnapTime = now;
}