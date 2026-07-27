#ifndef WOW_SOURCE_GAME_GAMETIME_H
#define WOW_SOURCE_GAME_GAMETIME_H

#include <stpl.h>

#include "Base/Handle.h"
#include "WowTime.h"

DECLARE_DERIVED_HANDLE(HGAMETIMECALLBACK, HOBJECT);

struct GAMETIMECBSTRUCT : public TSLinkedNode<GAMETIMECBSTRUCT>, public CHandleObject {
  void *userData;
  void(__stdcall *callback)(const WowTime &, void *);
};

struct TIMESTAMPSTRUCT : public TSHashObject<TIMESTAMPSTRUCT, HASHKEY_NONE> {
  TSList<GAMETIMECBSTRUCT, TSGetLink<GAMETIMECBSTRUCT> > callbackList;
};

class CGameTime : public WowTime {
 public:
  CGameTime();

  int   GetTimeBias() const;
  int   GetDateBias() const;
  void  Destroy();
  void  SetTimeDateBias(int timeBias, int dateBias, bool update);
  void  GameTimeSetTime(const WowTime &time);
  void  GameTimeUpdate(float elapsedSeconds);
  void  GameTimeSync(const WowTime &time, bool reset);
  void  GameTimeSync(bool reset);
  float GameTimeSetMinutesPerSecond(float minutesPerSecond);
  float GameTimeGetMinutesPerSecond();
  bool  IsDayTime();
  bool  IsNightTime();
  float GameTimeGetDayProgression();
  HGAMETIMECALLBACK GameTimeRegisterCallback(const WowTime &time, void(__stdcall *callback)(const WowTime &, void *), void *user);
  void  GameTimeUnregisterCallback(HGAMETIMECALLBACK callbackHandle);
  unsigned int MinutesSinceBoot();

  unsigned long m_lastTick;

 private:
  void TickMinute();
  void PerformCallbacks(int minutes);

  int                                        m_timeBias;
  int                                        m_dateBias;
  unsigned int                               m_gameMinutesElapsed;
  float                                      m_gameMinutesPerRealSecond;
  float                                      m_gameMinutesThisTick;
  unsigned int                               m_timeDifferential;
  unsigned int                               m_lastTickMinute;
  float                                      m_dayProgression;
  TSHashTable<TIMESTAMPSTRUCT, HASHKEY_NONE> m_callbackLists;
};

extern CGameTime g_clientGameTime;

int ClientGameTimeTickHandler(const void *data, void *__formal);
void ClientInitializeGameTime();
void ClientDestroyGameTime();
void SetGameTimeForcedChangeCallback(
    int set, void(*callback)(unsigned int oldTime, unsigned int newTime)
);

#endif
