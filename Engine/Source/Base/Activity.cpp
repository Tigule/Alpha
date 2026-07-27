#include "Activity.h"

#include "Os/OsTime.h"

#include <string.h>
#include <storm.h>
#include <stpl.h>

const unsigned int MAX_STACK_DEPTH = 100;

static TSGrowableArray<ACTIVITY> s_activityStack;
static __int64                   s_firstTime;
static __int64                   s_lastTime;
static __int64                   s_periodStartTimeClocks;
static __int64                   s_periodElapsedTimeClocks;
static __int64                   s_totalTime[ACTIVITIES];
static unsigned int              s_totalCalls[ACTIVITIES];
static double                    s_timeScale;
static int                       s_timeScaleComputed;

static void ActivitySuspend() {
  __int64 currTime = OsGetAsyncTimeClocks();

  if (s_activityStack.Count()) {
    ACTIVITY activity = *s_activityStack.Top();
    s_totalTime[activity] += currTime - s_lastTime;
    s_lastTime = currTime;
  }
}

static void ActivityResume() {
  s_lastTime = OsGetAsyncTimeClocks();
}

void ActivityBegin(ACTIVITY activity) {
  ACTIVITY previousActivity;
  __int64  currentTime;

  ASSERT(activity < ACTIVITIES);
  ASSERT(activity != ACTIVITY_OTHER);

  previousActivity = ACTIVITY_OTHER;

  if (s_activityStack.Count()) {
    previousActivity = *s_activityStack.Top();
  }

  currentTime = OsGetAsyncTimeClocks();
  s_totalTime[previousActivity] += currentTime - s_lastTime;
  s_lastTime = currentTime;

  if (!s_activityStack.Count()) {
    s_firstTime = currentTime;
  }

  *s_activityStack.New() = activity;

  ASSERT(s_activityStack.Count() <= MAX_STACK_DEPTH);
}

void ActivityEnd(ACTIVITY activity) {
  __int64 currTime = OsGetAsyncTimeClocks();

  FATALASSERT(activity == *s_activityStack.Top());

  s_activityStack.SetCount(s_activityStack.Count() - 1);
  s_totalTime[activity] += currTime - s_lastTime;
  ++s_totalCalls[activity];
  s_lastTime = currTime;
}

float ActivityGetTimePercent(ACTIVITY activity) {
  if (s_periodStartTimeClocks && !s_periodElapsedTimeClocks) {
    s_periodElapsedTimeClocks = OsGetAsyncTimeClocks() - s_periodStartTimeClocks;
  }

  if (!s_periodElapsedTimeClocks) {
    return 0.0f;
  }

  return (float)((double)s_totalTime[activity] * 100.0 / (double)s_periodElapsedTimeClocks);
}

float ActivityGetCalls(ACTIVITY activity) {
  return (float)s_totalCalls[activity];
}

float ActivityGetTime(ACTIVITY activity) {
  if (!s_timeScaleComputed) {
    ActivitySuspend();
    s_timeScale = (double)OsGetAsyncClocksDivisor() * 1000.0;
    s_timeScaleComputed = 1;
    ActivityResume();
  }

  return (float)((double)s_totalTime[activity] * s_timeScale);
}

void ActivityResetTimes() {
  unsigned int i;

  memset(s_totalCalls, 0, sizeof(s_totalCalls));

  for (i = 0; i < ACTIVITIES; ++i) {
    s_totalTime[i] = 0;
  }

  s_periodStartTimeClocks = OsGetAsyncTimeClocks();
  s_periodElapsedTimeClocks = 0;
}
