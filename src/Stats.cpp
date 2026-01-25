// Statistics counting
#include <Preferences.h>
#include <sstream>
#include <iostream>
#include "Config.h"
#include "Defines.h"
#include "Stats.h"
#include "DeviceConfig.h"
#include "Prefs.h"
#include <Debug.h>

ControllerReport ResetAllCurrentStats()
{
  for (int i = 0; i < AllStats_Count; i++)
    AllStats[i]->ResetCurrentCounts();

  return DontReportToController;
}

void UpdateSecondStats(int second)
{
#ifdef DEBUG_MARKS
  Debug::Mark(30, __LINE__, __FILE__, __func__);
#endif

  for (int i = 0; i < AllStats_Count; i++)
    AllStats[i]->SecondPassed(second);
}

void UpdateSubSecondStats()
{
#ifdef DEBUG_MARKS
  Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

  for (int i = 0; i < AllStats_Count; i++)
    AllStats[i]->SubSecondPassed();
}

Stats::Stats(const char *description, const char *prefsKey) : Description(description) { InitPrefsKey(prefsKey); };
Stats::Stats(const char *description, const char *prefsKey, float elasticReductionRate, float elasticMax) : Description(description), ElasticReductionRate(elasticReductionRate), ElasticMax(elasticMax) { InitPrefsKey(prefsKey); };
Stats::Stats(const char *description, const char *prefsKey, Stats *nextStats) : Description(description), NextStat(nextStats) { InitPrefsKey(prefsKey); };
Stats::Stats(const char *description, const char *prefsKey, float elasticReductionRate, float elasticMax, Stats *nextStats) : Description(description), ElasticReductionRate(elasticReductionRate), ElasticMax(elasticMax), NextStat(nextStats) { InitPrefsKey(prefsKey); };

void Stats::InitPrefsKey(const char *prefsKey)
{
  // Prepend Stats. on it
  const char *prefix = "Stats.";
  size_t len = snprintf(nullptr, 0, "%s%s", prefix, prefsKey) + 1;
  PrefsKey = new char[len]; // Not tracked, we never free this
  snprintf(PrefsKey, len, "%s%s", prefix, prefsKey);
}

void Stats::AddCount()
{
  // Serial.println("Adding count to " + String(Description));
  Current_SecondCount++;
  Current_TotalCount++;
  Session_TotalCount++;
  Ever_TotalCount++;

  if (NextStat != nullptr)
    NextStat->AddCount();
}

void Stats::ResetCurrentCounts()
{
  // Wipe variables
  Current_SecondCount = 0;
  Current_TotalCount = 0;
  Current_MaxPerSecond = 0;
  Current_MaxPerSecondOverLastMinute = 0;

  // Wipe minute counts
  for (int i = 0; i < STATS_MINUTE_COUNT; i++)
    Counts[i] = 0;

  // Wipe sub counts
  for (int i = 0; i < STATS_SUBCOUNTS_COUNT; i++)
    SubCounts[i] = 0;

  SubCountPos = 0;
}

void Stats::ResetSessionCounts()
{
  int Session_TotalCount = 0;
}

void Stats::ResetEverCounts()
{
  Ever_TotalCount = 0;
}

void Stats::LoadFromFlash()
{
}

void Stats::SaveToFlash()
{
}

void Stats::SecondPassed(int Second)
{
  // Example comments below are based on a SubSecond setting of 10 (i.e. 10 10th/s of a second)

  // So what's going on...
  // Work out the highest count over a 1 second period over previous almost 2 seconds time
  // i.e. we don't count per second...
  // What if we have a second with a count of 15, and the next second a count of 20? Max is 20 yes, but...
  // What if that first second, 5 took place in the first 1/2 of this second, and 10 of the count took place in the last 10th of a second
  // + 17 of the next count took place in the first 10th of the next second and the remaining 3 at the end of the second second?
  // ...There will have been a second long period in the middle overlapping both the +10 and the +17 - meaning our max value within a second period is 27, not 20!
  // Putting in dynamic calculations for this with infinite flexibility = extra overhead and complexity,
  // so we go for a middle ground of slicing up each second, and when working out per second counts,
  // we use a little sliding window to check all the counts within each slice in second long sums, to find
  // a decent approx most per second that does the job. Theoretically if you put the resolution of this to be higher than the fastest possible
  // input speed, then job done, but down to like 10th of a second generally works.
  // Remembering this is an embedded device and we want to be pretty speedy in overhead, keep that latency low!

  // SecondPassed always called after UpdateSubSecond, so UpDownSubCountPos will sit 1 after previous seconds end SubSecond point sits
  // means current UpDownSubCountPos position is effectively the start position of the oldest second window we want to measure
  // Handy!

  int maxCount = 0;
  int tempCount = 0;

  int minuteCount = STATS_MINUTE_COUNT;
  int subCounts_Count = STATS_SUBCOUNTS_COUNT;
  int subSecond_Count = SUB_SECOND_COUNT;

  int startPos = SubCountPos + 1; // +1 cause we want to make sure we aren't starting with the entire previous second, only the previous e.g. 9/10ths plus first 10th of new second
  if (startPos == subCounts_Count)
    startPos = 0;

  int pos = startPos;
  for (int i = 0; i < subCounts_Count; i++)
  {
    tempCount += SubCounts[pos++];

    if (pos == subCounts_Count)
      pos = 0;
  }

  maxCount = tempCount;
  // We have our first second's worth count, we now only have to remove the first SubValue and add the next SubValue at the end to calculate next potential second total

  int endPos = startPos + subSecond_Count;
  if (endPos > subCounts_Count)
    endPos -= subCounts_Count;

  int loops = subSecond_Count - 1; // Skip a loop, i.e. the one above thats already added everything together for an initial value

  for (int i = 0; i < subSecond_Count; i++)
  {
    tempCount -= SubCounts[startPos++];
    tempCount += SubCounts[endPos++];

    if (startPos >= subCounts_Count)
      startPos -= subCounts_Count;

    if (endPos >= subCounts_Count)
      endPos -= subCounts_Count;

    if (tempCount > maxCount)
      maxCount = tempCount;
  }

  Counts[Second] = maxCount;
  Current_CrossSecondCount = maxCount;

  if (Current_MaxPerSecond < maxCount)
    Current_MaxPerSecond = maxCount;

  if (Session_MaxPerSecond < maxCount)
    Session_MaxPerSecond = maxCount;

  if (Ever_MaxPerSecond < maxCount)
    Ever_MaxPerSecond = maxCount;

  Current_MaxPerSecondOverLastMinute = 0;
  for (int i = 0; i < minuteCount; i++)
  {
    if (Counts[i] > Current_MaxPerSecondOverLastMinute)
      Current_MaxPerSecondOverLastMinute = Counts[i];
  }
}

// float Stats::ElasticCount;
// float Stats::ElasticReductionRate = 1.0;
// float Stats::ElasticMax = 20.0;
// float Stats::ElasticPercentage;

void Stats::SubSecondPassed()
{
  if (ElasticReductionRate > 0)
  {
    ElasticCount -= ElasticReductionRate;
    ElasticCount += Current_SecondCount;
    if (ElasticCount < 0)
      ElasticCount = 0;
    else if (ElasticCount > ElasticMax)
      ElasticCount = ElasticMax;

    ElasticPercentage = (float)ElasticCount / (float)ElasticMax;

    // Serial.println("Elastic [" + String(Description) + "]- Increase: " + String(Current_SecondCount) + ", Count: " + String(ElasticCount) + ", Percentage: " + String(ElasticPercentage) + ", Max: " + String(ElasticMax) + ", Reduction Rate: " + String(ElasticReductionRate));

    // delay(25);
  }

  SubCounts[SubCountPos++] = Current_SecondCount;
  if (SubCountPos == STATS_SUBCOUNTS_COUNT)
    SubCountPos = 0;

  // SubCountPos = 0;
  Current_SecondCount = 0;
}

// REMINDER: Max Preferences key length = 15 chars
// C_ Current value
// S_ Session value
// E_ Ever value
void Stats::SaveToPreferences()
{
  auto &prefs = Prefs::Handler;

  prefs.begin(PrefsKey);

  prefs.putInt("C_SecondCount", Current_SecondCount);
  prefs.putInt("C_TotalCount", Current_TotalCount);
  prefs.putInt("C_MaxPerSecond", Current_MaxPerSecond);
  prefs.putInt("C_MaxPerSecLMin", Current_MaxPerSecondOverLastMinute);
  prefs.putInt("S_TotalCount", Session_TotalCount);
  prefs.putInt("S_MaxPerSecond", Session_MaxPerSecond);
  prefs.putInt("E_TotalCount", Ever_TotalCount);
  prefs.putInt("E_MaxPerSecond", Ever_MaxPerSecond);

  prefs.end();
}

void Stats::LoadFromPreferences()
{
  auto &prefs = Prefs::Handler;

  prefs.begin(PrefsKey);

  Current_SecondCount = prefs.getInt("C_SecondCount", 0);
  Current_TotalCount = prefs.getInt("C_TotalCount", 0);
  Current_MaxPerSecond = prefs.getInt("C_MaxPerSecond", 0);
  Current_MaxPerSecondOverLastMinute = prefs.getInt("C_MaxPerSecLMin", 0);
  Session_TotalCount = prefs.getInt("S_TotalCount", 0);
  Session_MaxPerSecond = prefs.getInt("S_MaxPerSecond", 0);
  Ever_TotalCount = prefs.getInt("E_TotalCount", 0);
  Ever_MaxPerSecond = prefs.getInt("E_MaxPerSecond", 0);

  prefs.end();
}

// Part of a table
void Stats::WebDebug(std::ostringstream *stream)
{

  // String key = "stats." + String(Description) + ".";
  std::string key = "stats." + std::string(Description) + ".";

  // Serial.println("STATS: " + String(key));

  *stream
      << "<tr><th colspan='2'>" << key << "</th></tr>"
      << "<tr><td>" << key << "Current - Second Count</td><td>" << Current_SecondCount << "</td></tr>"
      << "<tr><td>" << key << "Current - Total Count</td><td>" << Current_TotalCount << "</td></tr>"
      << "<tr><td>" << key << "Current - Max Per Second</td><td>" << Current_MaxPerSecond << "</td></tr>"
      << "<tr><td>" << key << "Current - Max Per Second Over Last Minute</td><td>" << Current_MaxPerSecondOverLastMinute << "</td></tr>"
      << "<tr><td>" << key << "Session - Total Count</td><td>" << Session_TotalCount << "</td></tr>"
      << "<tr><td>" << key << "Session - Max Per Second</td><td>" << Session_MaxPerSecond << "</td></tr>"
      << "<tr><td>" << key << "Ever - TotalCount</td><td>" << Ever_TotalCount << "</td></tr>"
      << "<tr><td>" << key << "Ever - Max Per Second</td><td>" << Ever_MaxPerSecond << "</td></tr>";
}