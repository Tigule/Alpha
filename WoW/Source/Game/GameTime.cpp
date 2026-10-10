#include <Base/Base.h>
#include <WowConst.h>

#include "Game/GameTime.h"

#include "Os/OsTime.h"

#include <time.h>

static const float MAXMINUTES_PER_SECOND = 60.0f;

CGameTime::CGameTime()
    : m_timeBias(0),
      m_dateBias(0),
      m_gameMinutesElapsed(0),
      m_gameMinutesPerRealSecond(1.0f / 60.0f),
      m_gameMinutesThisTick(0.0f),
      m_timeDifferential(0),
      m_lastTickMinute(OsGetAsyncTimeMs()),
      m_dayProgression(0.0f) {
}

void CGameTime::Destroy() {
  m_callbackLists.Clear();
}

void CGameTime::SetTimeDateBias(int timeBias, int dateBias, bool update) {
  if (m_timeBias) {
    UINT minutes = GetHourAndMinutes() - m_timeBias;

    if (minutes < 1440) {
      minutes += 1440;
    } else {
      minutes %= 1440;
    }

    SetHourAndMinutes(minutes);
  }

  int oldDateBias = m_dateBias;
  m_timeBias = timeBias;

  if (oldDateBias) {
    SetDaysSinceEpoch(GetDaysSinceEpoch() - m_dateBias);
  }

  m_dateBias = dateBias;

  if (update) {
    GameTimeSetTime(*this);
  }
}

void CGameTime::GameTimeSetTime(const WowTime &time) {
  WowTime biasTime = time;

  if (m_timeBias) {
    int minutes = biasTime.GetHourAndMinutes() + m_timeBias;

    if (minutes < 0) {
      minutes += 1440;
    } else {
      minutes = (UINT)minutes % 1440;
    }

    biasTime.SetHourAndMinutes(minutes);
  }

  if (m_dateBias) {
    biasTime.SetDaysSinceEpoch(biasTime.GetDaysSinceEpoch() + m_dateBias);
  }

  (WowTime &)*this = biasTime;

  if (!m_minute) {
    m_minute = 59;
    if (!m_hour) {
      m_hour = 23;
    } else {
      --m_hour;
    }
  } else {
    --m_minute;
  }

  TickMinute();
}

void CGameTime::GameTimeUpdate(float elapsedSeconds) {
  UINT timeDifferential = m_timeDifferential;
  m_gameMinutesThisTick = elapsedSeconds * m_gameMinutesPerRealSecond + m_gameMinutesThisTick;

  if (timeDifferential && m_gameMinutesThisTick >= 1.0) {
    UINT correction = m_gameMinutesThisTick;
    if (timeDifferential < correction) {
      correction = timeDifferential;
    }
    m_timeDifferential = timeDifferential - correction;
    m_gameMinutesThisTick -= correction;
    FATALASSERT(m_gameMinutesThisTick >= 0.0f);
  }

  while (m_gameMinutesThisTick >= 1.0f) {
    m_gameMinutesThisTick -= 1.0f;
    TickMinute();
  }
}

void CGameTime::GameTimeSync(const WowTime &time, bool reset) {
  WowTime biasTime = time;

  if (m_timeBias) {
    int minutes = biasTime.GetHourAndMinutes() + m_timeBias;

    if (minutes < 0) {
      minutes += 1440;
    } else {
      minutes = (UINT)minutes % 1440;
    }

    biasTime.SetHourAndMinutes(minutes);
  }

  if (m_dateBias) {
    biasTime.SetDaysSinceEpoch(biasTime.GetDaysSinceEpoch() + m_dateBias);
  }

  int delta = biasTime.GetHourAndMinutes() - GetHourAndMinutes();
  if (reset || delta > 0) {
    UINT forward = (UINT)(delta + 1440) % 1440;
    while (forward > 0) {
      TickMinute();
      --forward;
    }
    delta = 0;
  } else {
    delta = -delta;
  }

  (WowTime &)*this = biasTime;

  if (delta) {
    m_timeDifferential += delta;
    SetHourAndMinutes((UINT)(GetHourAndMinutes() + delta) % 1440);
  }
}

float CGameTime::GameTimeGetMinutesPerSecond() {
  return m_gameMinutesPerRealSecond;
}

void CGameTime::GameTimeSync(bool reset) {
  time_t seconds;

  time(&seconds);

  struct tm *localTime = localtime(&seconds);

  WowTime time;

  time.m_minute = localTime->tm_min;
  time.m_hour = localTime->tm_hour;
  time.m_weekday = localTime->tm_wday;
  time.m_monthDay = localTime->tm_mday - 1;
  time.m_month = localTime->tm_mon;
  time.m_year = localTime->tm_year - 100;

  if (reset) {
    GameTimeSetTime(time);
  } else {
    GameTimeSync(time, false);
  }
}

HGAMETIMECALLBACK CGameTime::GameTimeRegisterCallback(const WowTime &time, void(__stdcall *callback)(const WowTime &, LPVOID), LPVOID user) {
  VALIDATEBEGIN;
  VALIDATE(callback);
  VALIDATEEND;

  if (time.m_hour < 0 || time.m_minute < 0) {
    return 0;
  }

  GAMETIMECBSTRUCT *newCallback = NEWHANDLE(HGAMETIMECALLBACK, GAMETIMECBSTRUCT);

  newCallback->userData = user;
  newCallback->callback = callback;

  int              timeStamp = time.GetHourAndMinutes();
  HASHKEY_NONE     key;
  TIMESTAMPSTRUCT *timestamp = m_callbackLists.Ptr(timeStamp, key);

  if (!timestamp) {
    timestamp = m_callbackLists.New(timeStamp, key, 0, 0);
  }

  timestamp->callbackList.LinkNode(newCallback, LIST_TAIL, 0);

  return CREATEHANDLE(HGAMETIMECALLBACK, newCallback);
}

void CGameTime::GameTimeUnregisterCallback(HGAMETIMECALLBACK callbackHandle) {
  HandleClose(callbackHandle);
}

float CGameTime::GameTimeSetMinutesPerSecond(float minutesPerSecond) {
  float oldMinutesPerSecond = m_gameMinutesPerRealSecond;
  m_gameMinutesPerRealSecond = min(MAXMINUTES_PER_SECOND, max(1.0f / 60.0f, minutesPerSecond));

  return oldMinutesPerSecond;
}

float CGameTime::GameTimeGetDayProgression() {
  float progression = m_gameMinutesPerRealSecond * ((OsGetAsyncTimeMs() - m_lastTickMinute) * 0.001f) + m_dayProgression;

  while (progression > 1440.0f) {
    progression -= 1440.0f;
  }

  return progression * (1.0f / 1440.0f);
}

void CGameTime::TickMinute() {
  int minutes = (UINT)(GetHourAndMinutes() + 1) % 1440;

  SetHourAndMinutes(minutes);
  ++m_gameMinutesElapsed;
  PerformCallbacks(minutes);
  m_lastTickMinute = OsGetAsyncTimeMs();
  m_dayProgression = minutes + m_gameMinutesThisTick;
}

void CGameTime::PerformCallbacks(int minutes) {
  HASHKEY_NONE     key;
  TIMESTAMPSTRUCT *timestamp = m_callbackLists.Ptr(minutes, key);
  if (!timestamp) {
    return;
  }

  ITERATELIST(GAMETIMECBSTRUCT, timestamp->callbackList, curr) {
    ASSERT(curr->callback);
    curr->callback(*this, curr->userData);
  }
}
