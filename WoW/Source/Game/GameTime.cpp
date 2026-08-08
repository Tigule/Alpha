#include <Base/Base.h>
#include <WowConst.h>

#include "Game/GameTime.h"

#include "Os/OsTime.h"

#include <time.h>

TIMESTAMPSTRUCT::~TIMESTAMPSTRUCT() {
}

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
      minutes = static_cast<UINT>(minutes) % 1440;
    }

    biasTime.SetHourAndMinutes(minutes);
  }

  if (m_dateBias) {
    biasTime.SetDaysSinceEpoch(biasTime.GetDaysSinceEpoch() + m_dateBias);
  }

  static_cast<WowTime &>(*this) = biasTime;

  if (m_minute) {
    --m_minute;
  } else {
    m_minute = 59;
    if (m_hour) {
      --m_hour;
    } else {
      m_hour = 23;
    }
  }

  TickMinute();
}

void CGameTime::GameTimeUpdate(float elapsedSeconds) {
  UINT timeDifferential = m_timeDifferential;
  m_gameMinutesThisTick = elapsedSeconds * m_gameMinutesPerRealSecond + m_gameMinutesThisTick;

  if (timeDifferential && m_gameMinutesThisTick >= 1.0f) {
    UINT correction = static_cast<UINT>(m_gameMinutesThisTick);
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
      minutes = static_cast<UINT>(minutes) % 1440;
    }

    biasTime.SetHourAndMinutes(minutes);
  }

  if (m_dateBias) {
    biasTime.SetDaysSinceEpoch(biasTime.GetDaysSinceEpoch() + m_dateBias);
  }

  int delta = biasTime.GetHourAndMinutes() - GetHourAndMinutes();
  if (reset || delta > 0) {
    UINT forward = static_cast<UINT>(delta + 1440) % 1440;
    while (forward) {
      TickMinute();
      --forward;
    }
    delta = 0;
  } else {
    delta = -delta;
  }

  static_cast<WowTime &>(*this) = biasTime;

  if (delta) {
    m_timeDifferential += delta;
    SetHourAndMinutes(static_cast<UINT>(GetHourAndMinutes() + delta) % 1440);
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
  FATALASSERT(callback);

  if (time.m_hour < 0 || time.m_minute < 0) {
    return 0;
  }

  GAMETIMECBSTRUCT *newCallback = new (SMemAlloc(sizeof(GAMETIMECBSTRUCT), "HGAMETIMECALLBACK", SERR_LINECODE_OBJECT, 0)) GAMETIMECBSTRUCT;

  newCallback->userData = user;
  newCallback->callback = callback;

  int              hourAndMinutes = time.GetHourAndMinutes();
  HASHKEY_NONE     key;
  TIMESTAMPSTRUCT *timestamp = m_callbackLists.Ptr(hourAndMinutes, key);

  if (!timestamp) {
    timestamp = m_callbackLists.New(hourAndMinutes, key, 0, 0);
  }

  timestamp->callbackList.LinkNode(newCallback, LIST_HEAD, 0);

  return static_cast<HGAMETIMECALLBACK>(HandleCreate(newCallback, "HGAMETIMECALLBACK"));
}

void CGameTime::GameTimeUnregisterCallback(HGAMETIMECALLBACK callbackHandle) {
  HandleClose(callbackHandle);
}

float CGameTime::GameTimeSetMinutesPerSecond(float minutesPerSecond) {
  float oldMinutesPerSecond = m_gameMinutesPerRealSecond;
  float clamped = minutesPerSecond;

  if (clamped < 1.0f / 60.0f) {
    clamped = 1.0f / 60.0f;
  }
  if (clamped > 60.0f) {
    clamped = 60.0f;
  }

  m_gameMinutesPerRealSecond = clamped;
  return oldMinutesPerSecond;
}

float CGameTime::GameTimeGetDayProgression() {
  float progression = (OsGetAsyncTimeMs() - m_lastTickMinute) * 0.001f * m_gameMinutesPerRealSecond + m_dayProgression;

  while (progression > 1440.0f) {
    progression -= 1440.0f;
  }

  return progression * (1.0f / 1440.0f);
}

void CGameTime::TickMinute() {
  int minutes = static_cast<UINT>(GetHourAndMinutes() + 1) % 1440;

  SetHourAndMinutes(minutes);
  ++m_gameMinutesElapsed;
  PerformCallbacks(minutes);
  m_lastTickMinute = OsGetAsyncTimeMs();
  m_dayProgression = minutes + m_gameMinutesThisTick;
}

void CGameTime::PerformCallbacks(int minutes) {
  if (m_callbackLists.m_slotmask == 0xFFFFFFFF) {
    return;
  }

  UINT             slot = minutes & m_callbackLists.m_slotmask;
  TIMESTAMPSTRUCT *timestamp = m_callbackLists.m_slotlistarray[slot].Head();

  while (reinterpret_cast<long>(timestamp) > 0 && timestamp->GetHashValue() != minutes) {
    timestamp = m_callbackLists.m_slotlistarray[slot].RawNext(timestamp);
  }

  if (reinterpret_cast<long>(timestamp) <= 0) {
    return;
  }

  ITERATELIST(GAMETIMECBSTRUCT, timestamp->callbackList, curr) {
    ASSERT(curr->callback);
    curr->callback(*this, curr->userData);
  }
}
