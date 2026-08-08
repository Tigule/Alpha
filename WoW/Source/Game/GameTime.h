#ifndef WOW_SOURCE_GAME_GAMETIME_H
#define WOW_SOURCE_GAME_GAMETIME_H

#include <stpl.h>

#include "Base/Handle.h"
#include "WowTime.h"

DECLARE_DERIVED_HANDLE(HGAMETIMECALLBACK, HOBJECT);

NODEDECL(GAMETIMECBSTRUCT), public CHandleObject {
  GAMETIMECBSTRUCT() : userData(0), callback(0) {
  }
  GAMETIMECBSTRUCT(const GAMETIMECBSTRUCT &);

  LPVOID userData;
  void(__stdcall * callback)(const WowTime &, LPVOID);
};

struct TIMESTAMPSTRUCT : public TSHashObject<TIMESTAMPSTRUCT, HASHKEY_NONE> {
  ~TIMESTAMPSTRUCT();

  LISTDECL(GAMETIMECBSTRUCT, callbackList);
};

class CGameTime : public WowTime {
 public:
  CGameTime();

  int               GetTimeBias() const;
  int               GetDateBias() const;
  void              Destroy();
  void              SetTimeDateBias(int timeBias, int dateBias, bool update);
  void              GameTimeSetTime(const WowTime &time);
  void              GameTimeUpdate(float elapsedSeconds);
  void              GameTimeSync(const WowTime &time, bool reset);
  void              GameTimeSync(bool reset);
  float             GameTimeSetMinutesPerSecond(float minutesPerSecond);
  float             GameTimeGetMinutesPerSecond();
  bool              IsDayTime();
  bool              IsNightTime();
  float             GameTimeGetDayProgression();
  HGAMETIMECALLBACK GameTimeRegisterCallback(const WowTime &time, void(__stdcall *callback)(const WowTime &, LPVOID), LPVOID user);
  void              GameTimeUnregisterCallback(HGAMETIMECALLBACK callbackHandle);
  UINT              MinutesSinceBoot();

  DWORD m_lastTick;

 private:
  void TickMinute();
  void PerformCallbacks(int minutes);

  int                                        m_timeBias;
  int                                        m_dateBias;
  UINT                                       m_gameMinutesElapsed;
  float                                      m_gameMinutesPerRealSecond;
  float                                      m_gameMinutesThisTick;
  UINT                                       m_timeDifferential;
  UINT                                       m_lastTickMinute;
  float                                      m_dayProgression;
  TSHashTable<TIMESTAMPSTRUCT, HASHKEY_NONE> m_callbackLists;
};

extern CGameTime g_clientGameTime;

BOOL ClientGameTimeTickHandler(LPCVOID data, LPVOID);
void ClientInitializeGameTime();
void ClientDestroyGameTime();
void SetGameTimeForcedChangeCallback(int set, void (*callback)(UINT oldTime, UINT newTime));

#endif
