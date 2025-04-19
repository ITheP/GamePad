// Statistics counting
#include "Config.h"
#include "Stats.h"

int UpDownCurrentCount = 0;
int UpDownCounts[UpDownCounts_Count];    
int UpDownSubCounts[UpDownSubCounts_Count];
int UpDownSubCountPos = 0;

// Stats
long UpDownTotalCount;
long UpDownMaxPerSecondCount;
long UpDownMaxPerSecondOverLastMinuteCount;
long UpDownMaxPerSecondEver;

void UpDownCount_Reset() {
  UpDownTotalCount = 0;
  UpDownMaxPerSecondCount = 0;
  UpDownMaxPerSecondOverLastMinuteCount = 0;
  UpDownMaxPerSecondEver = 0;

  for (int i = 0; i < UpDownCounts_Count; i++)
    UpDownCounts[i] = 0;

  for (int i = 0; i < UpDownSubCounts_Count; i++)
    UpDownSubCounts[i] = 0;

  UpDownSubCountPos = 0;
}

// Should be called each SubSsecondRollover
void UpDownCount_UpdateSubSecond() {
  UpDownSubCounts[UpDownSubCountPos++] = UpDownCurrentCount;
  if (UpDownSubCountPos == UpDownSubCounts_Count)
    UpDownSubCountPos = 0;

  UpDownCurrentCount = 0;
}

void UpDownCount_SecondPassed(int Second) {
  // Example comments below are based on a SubSecond setting of 10 (i.e. 10 10th/s of a second)

  // So whats going on...
  // Work out the highest count over a 1 second period over previous almost 2 seconds time
  // i.e. we don't count per second...
  // What if we have a second with a count of 15, and the next second a count of 20? Max is 20 yes, but...
  // What if that first second, 5 took place in the first 1/2 of this second, and 10 of the count took place in the last 10th of a second
  // + 17 of the next count took place in the first 10th of the next second and the remaining 3 at the end of the second second?
  // ...There will have been a second long period in the middle overlapping both the +10 and the +17 - meaning our max value within a second period is 27, not 20!
  // Putting in dynamic calculations for this with infinite flexibiliy = extra overhead and complexity,
  // so we go for a middle ground of slicing up each second, and when working out per second counts,
  // we use a little sliding window to check all the counts within each slice in second long sums, to find
  // a decent approx most per second that does the job. Theoretically if you put the resolution of this to be higher than the fastest possible
  // input speed, then job done, but down to like 10th of a second generally works.
  // Remembering this is an embedded device and we want to be pretty speedy in overhead, keep that latency low!

  // SecondPassed always called after UpdateSubSecond, so UpDownSubCountPos will sit 1 after previous seconds end SubSecond point sits
  // means current UpDownSubCountPos position is effectivly the start position of the oldest second window we want to measure
  // Handy!

  int maxCount = 0;
  int tempCount = 0;

  int startPos = UpDownSubCountPos + 1;  // +1 cause we want to make sure we arent starting with the entire previous second, only the previous e.g. 9/10ths plua first 10th of new second
  if (startPos == UpDownSubCounts_Count)
    startPos = 0;

  int pos = startPos;
  for (int i = 0; i < SUB_SECOND_COUNT; i++) {
    tempCount += UpDownSubCounts[pos++];

    if (pos == UpDownSubCounts_Count)
      pos = 0;
  }

  maxCount = tempCount;
  // We have our first second's worth count, we now only have to remove the first SubValue and add the next SubValue at the end to calculate next potential second total

  int endPos = startPos + SUB_SECOND_COUNT;
  if (endPos > UpDownSubCounts_Count)
    endPos -= UpDownSubCounts_Count;

  int loops = SUB_SECOND_COUNT - 1;  // Skip a loop, i.e. the one above thats already added everything together for an initial value

  for (int i = 0; i < SUB_SECOND_COUNT; i++) {
    tempCount -= UpDownSubCounts[startPos++];
    tempCount += UpDownSubCounts[endPos++];

    if (startPos >= UpDownSubCounts_Count)
      startPos -= UpDownSubCounts_Count;

    if (endPos >= UpDownSubCounts_Count)
      endPos -= UpDownSubCounts_Count;

    if (tempCount > maxCount)
      maxCount = tempCount;
  }

  UpDownCounts[Second] = maxCount;

  UpDownMaxPerSecondOverLastMinuteCount = 0;
  for (int i = 0; i < UpDownCounts_Count; i++) {
    if (UpDownCounts[i] > UpDownMaxPerSecondOverLastMinuteCount)
      UpDownMaxPerSecondOverLastMinuteCount = UpDownCounts[i];
  }
  UpDownMaxPerSecondCount = maxCount;

  if (maxCount > UpDownMaxPerSecondEver)
    UpDownMaxPerSecondEver = maxCount;
}

void UpDownCountClick(HatInput* input) {
  UpDownCurrentCount++;
  UpDownTotalCount++;
}