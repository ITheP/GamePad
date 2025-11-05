#pragma once

#include "Config.h"

#define STATS_MINUTE_COUNT 60                         // 60 second across a minute count
#define STATS_SUBCOUNTS_COUNT (SUB_SECOND_COUNT * 2) // Overall size of window that a 1 second sliding window is checked across to find out the actual max per second counts


ControllerReport ResetAllCurrentStats();
void UpdateSecondStats(int second);
void UpdateSubSecondStats();

// Stats can be used to e.g. count clicks on a single button,
// or be reused across more than one button (so e.g. a guitar strum bar up/down might add to an overall single total)

class Stats
{
public:
    // Constructors
    // description is generally used in Web interface, Serial debugging etc.
    // IMPORTANT: prefsKey please start with "Stats." and max length is 15 chars INCLUDING the "Stats.". This is used for saving stats into Preferences
    Stats(const char *description, const char *prefsKey);
    Stats(const char *description, const char *prefsKey, float elasticReductionRate, float elasticMax);
    Stats(const char *description, const char *prefsKey, Stats *nextStat);
    Stats(const char *description, const char *prefsKey, float elasticReductionRate, float elasticMax, Stats *nextStat);

    const char *Description;
    char *PrefsKey;
    
    // Elastic stat, useful for things like certain LED effects
    // Elastic as the value stretches towards the max value as counts are increased, but there is a constant reduction rate too so over time it will naturally return back to nothing
    float ElasticReductionRate;       // 0 to disable Elastic calculations
    float ElasticMax;
    float ElasticPercentage;

    // Stats for current
    int Current_SecondCount;
    int Current_TotalCount;
    //int Current_PerSecondCount;
    int Current_MaxPerSecond;
    int Current_MaxPerSecondOverLastMinute;
    //int Current_MaxPerSecondEver;

    int Current_CrossSecondCount;

    // Stats for current session (since power on)
    int Session_TotalCount;
    //int Session_CountPerSecond;
    int Session_MaxPerSecond;

    // Stats ever (including older sessions, if saved to flash)
    int Ever_TotalCount;
    //int Ever_CountPerSecond;
    int Ever_MaxPerSecond;

    // TODO: Design Choice - GetSessionCountPerSecond = previous GetSessionCountPerSecond + CountPerSecond - SessionCountPerSecond
    // is only updated CountPerSecond is reset, or device is powered down
    // Same with EverCounts - update when CountPerSecond is reset, or device powered down

// TODO: Second Operation (e.g. draw to screen)
// TODO: SubSecond Operation (e.g. draw to screen)

    //int TotalCount;
    //int MaxPerSecondCount;
    //int TotalEverCount;

    // Chain stats together - handy for e.g. keeping track of a single stat, but also stat combinations that add multiple inputs
    // E.g. a Hat input might have individual stats for each direction, but also a combined stat for the whole hat
    // E.g. a strum bar on a guitar controller might have individual stats for up/down, but also a combined stat for the whole strum bar
    Stats *NextStat; // Linked list of stats

    inline void Propagate()
    {
        if (NextStat != nullptr)
            NextStat->AddCount();
    }

    void LoadFromPreferences();
    void SaveToPreferences();
    void WebDebug(std::ostringstream *stream);

    void AddCount();

    void ResetCurrentCounts();
    void ResetSessionCounts();
    void ResetEverCounts();

    void LoadFromFlash();
    void SaveToFlash();

    void SecondPassed(int Second);
    void SubSecondPassed();

private:
    unsigned long StartTime;

    int Counts[STATS_MINUTE_COUNT];
    int SubCounts[STATS_SUBCOUNTS_COUNT];
    int SubCountPos = 0;
    float ElasticCount;

    void InitPrefsKey(const char* prefsKey);
};