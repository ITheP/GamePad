// Statistics counting
#include <Preferences.h>
#include "Config.h"
#include "Stats.h"
#include "DeviceConfig.h"
#include "Prefs.h"

ControllerReport ResetAllCurrentStats()
{
  for (int i = 0; i < AllStats_Count; i++)
    AllStats[i]->ResetCurrentCounts();

    return DontReportToController;
}

void UpdateSecondStats(int second)
{
  for (int i = 0; i < AllStats_Count; i++)
    AllStats[i]->SecondPassed(second);
}

void UpdateSubSecondStats()
{
  for (int i = 0; i < AllStats_Count; i++)
    AllStats[i]->SubSecondPassed();
}

Stats::Stats(const char *description) : Description(description) {};
Stats::Stats(const char *description, float elasticReductionRate, float elasticMax) : Description(description), ElasticReductionRate(elasticReductionRate), ElasticMax(elasticMax) {};
Stats::Stats(const char *description, Stats *nextStats) : Description(description), NextStat(nextStats) {};
Stats::Stats(const char *description, float elasticReductionRate, float elasticMax, Stats *nextStats) : Description(description), ElasticReductionRate(elasticReductionRate), ElasticMax(elasticMax), NextStat(nextStats) {};

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

  // So whats going on...
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

    //Serial.println("Elastic [" + String(Description) + "]- Increase: " + String(Current_SecondCount) + ", Count: " + String(ElasticCount) + ", Percentage: " + String(ElasticPercentage) + ", Max: " + String(ElasticMax) + ", Reduction Rate: " + String(ElasticReductionRate));

    //delay(25);
  }

  SubCounts[SubCountPos++] = Current_SecondCount;
  if (SubCountPos == STATS_SUBCOUNTS_COUNT)
    SubCountPos = 0;

  // SubCountPos = 0;
  Current_SecondCount = 0;
}

void Stats::LoadFromPreferences()
{
  String key = "stats." + String(Description) + ".";

  Prefs::Handler.putInt((key + "Current_SecondCount").c_str(), Current_SecondCount);
  Prefs::Handler.putInt((key + "Current_TotalCount").c_str(), Current_TotalCount);
  Prefs::Handler.putInt((key + "Current_MaxPerSecond").c_str(), Current_MaxPerSecond);
  Prefs::Handler.putInt((key + "Current_MaxPerSecondOverLastMinute").c_str(), Current_MaxPerSecondOverLastMinute);
  Prefs::Handler.putInt((key + "Session_TotalCount").c_str(), Session_TotalCount);
  Prefs::Handler.putInt((key + "Session_MaxPerSecond").c_str(), Session_MaxPerSecond);
  Prefs::Handler.putInt((key + "Ever_TotalCount").c_str(), Ever_TotalCount);
  Prefs::Handler.putInt((key + "Ever_MaxPerSecond").c_str(), Ever_MaxPerSecond);
}

void Stats::SaveToPreferences()
{
  String key = "stats." + String(Description) + ".";

  int Current_SecondCount = Prefs::Handler.getInt((key + "Current_SecondCount").c_str(), 0);
  int Current_TotalCount = Prefs::Handler.getInt((key + "Current_TotalCount").c_str(), 0);
  int Current_MaxPerSecond = Prefs::Handler.getInt((key + "Current_MaxPerSecond").c_str(), 0);
  int Current_MaxPerSecondOverLastMinute = Prefs::Handler.getInt((key + "Current_MaxPerSecondOverLastMinute").c_str(), 0);
  int Session_TotalCount = Prefs::Handler.getInt((key + "Session_TotalCount").c_str(), 0);
  int Session_MaxPerSecond = Prefs::Handler.getInt((key + "Session_MaxPerSecond").c_str(), 0);
  int Ever_TotalCount = Prefs::Handler.getInt((key + "Ever_TotalCount").c_str(), 0);
  int Ever_MaxPerSecond = Prefs::Handler.getInt((key + "Ever_MaxPerSecond").c_str(), 0);
}

// int UpDownCurrentCount = 0;
// int UpDownCounts[UpDownCounts_Count];
// int UpDownSubCounts[UpDownSubCounts_Count];
// int UpDownSubCountPos = 0;

// // Stats
// long UpDownTotalCount;
// long UpDownMaxPerSecondCount;
// long UpDownMaxPerSecondOverLastMinuteCount;
// long UpDownMaxPerSecondEver;

// void UpDownCount_Reset()
// {
//   UpDownTotalCount = 0;
//   UpDownMaxPerSecondCount = 0;
//   UpDownMaxPerSecondOverLastMinuteCount = 0;
//   UpDownMaxPerSecondEver = 0;

//   for (int i = 0; i < UpDownCounts_Count; i++)
//     UpDownCounts[i] = 0;

//   for (int i = 0; i < UpDownSubCounts_Count; i++)
//     UpDownSubCounts[i] = 0;

//   UpDownSubCountPos = 0;
// }

// // Should be called each SubSsecondRollover
// void UpDownCount_UpdateSubSecond()
// {
//   UpDownSubCounts[UpDownSubCountPos++] = UpDownCurrentCount;
//   if (UpDownSubCountPos == UpDownSubCounts_Count)
//     UpDownSubCountPos = 0;

//   UpDownCurrentCount = 0;
// }

// void UpDownCount_SecondPassed(int Second)
// {
//   // Example comments below are based on a SubSecond setting of 10 (i.e. 10 10th/s of a second)

//   // So whats going on...
//   // Work out the highest count over a 1 second period over previous almost 2 seconds time
//   // i.e. we don't count per second...
//   // What if we have a second with a count of 15, and the next second a count of 20? Max is 20 yes, but...
//   // What if that first second, 5 took place in the first 1/2 of this second, and 10 of the count took place in the last 10th of a second
//   // + 17 of the next count took place in the first 10th of the next second and the remaining 3 at the end of the second second?
//   // ...There will have been a second long period in the middle overlapping both the +10 and the +17 - meaning our max value within a second period is 27, not 20!
//   // Putting in dynamic calculations for this with infinite flexibility = extra overhead and complexity,
//   // so we go for a middle ground of slicing up each second, and when working out per second counts,
//   // we use a little sliding window to check all the counts within each slice in second long sums, to find
//   // a decent approx most per second that does the job. Theoretically if you put the resolution of this to be higher than the fastest possible
//   // input speed, then job done, but down to like 10th of a second generally works.
//   // Remembering this is an embedded device and we want to be pretty speedy in overhead, keep that latency low!

//   // SecondPassed always called after UpdateSubSecond, so UpDownSubCountPos will sit 1 after previous seconds end SubSecond point sits
//   // means current UpDownSubCountPos position is effectively the start position of the oldest second window we want to measure
//   // Handy!

//   int maxCount = 0;
//   int tempCount = 0;

//   int startPos = UpDownSubCountPos + 1; // +1 cause we want to make sure we arent starting with the entire previous second, only the previous e.g. 9/10ths plua first 10th of new second
//   if (startPos == UpDownSubCounts_Count)
//     startPos = 0;

//   int pos = startPos;
//   for (int i = 0; i < SUB_SECOND_COUNT; i++)
//   {
//     tempCount += UpDownSubCounts[pos++];

//     if (pos == UpDownSubCounts_Count)
//       pos = 0;
//   }

//   maxCount = tempCount;
//   // We have our first second's worth count, we now only have to remove the first SubValue and add the next SubValue at the end to calculate next potential second total

//   int endPos = startPos + SUB_SECOND_COUNT;
//   if (endPos > UpDownSubCounts_Count)
//     endPos -= UpDownSubCounts_Count;

//   int loops = SUB_SECOND_COUNT - 1; // Skip a loop, i.e. the one above thats already added everything together for an initial value

//   for (int i = 0; i < SUB_SECOND_COUNT; i++)
//   {
//     tempCount -= UpDownSubCounts[startPos++];
//     tempCount += UpDownSubCounts[endPos++];

//     if (startPos >= UpDownSubCounts_Count)
//       startPos -= UpDownSubCounts_Count;

//     if (endPos >= UpDownSubCounts_Count)
//       endPos -= UpDownSubCounts_Count;

//     if (tempCount > maxCount)
//       maxCount = tempCount;
//   }

//   UpDownCounts[Second] = maxCount;

//   UpDownMaxPerSecondOverLastMinuteCount = 0;
//   for (int i = 0; i < UpDownCounts_Count; i++)
//   {
//     if (UpDownCounts[i] > UpDownMaxPerSecondOverLastMinuteCount)
//       UpDownMaxPerSecondOverLastMinuteCount = UpDownCounts[i];
//   }
//   UpDownMaxPerSecondCount = maxCount;

//   if (maxCount > UpDownMaxPerSecondEver)
//     UpDownMaxPerSecondEver = maxCount;
// }

// void UpDownCountClick(HatInput *input)
// {
//   UpDownCurrentCount++;
//   UpDownTotalCount++;
// }