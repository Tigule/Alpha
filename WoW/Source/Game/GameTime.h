#ifndef WOW_SOURCE_GAME_GAMETIME_H
#define WOW_SOURCE_GAME_GAMETIME_H

#include <stpl.h>

#include "Base/Handle.h"
#include "WowTime.h"

DECLARE_DERIVED_HANDLE(HGAMETIMECALLBACK, HOBJECT);

NODEDECL(GAMETIMECBSTRUCT), public CHandleObject {
  LPVOID userData;
  void(__stdcall * callback)(const WowTime &, LPVOID);

  GAMETIMECBSTRUCT() : userData(0), callback(0) {
  }
  GAMETIMECBSTRUCT(const GAMETIMECBSTRUCT &);
};

struct TIMESTAMPSTRUCT : public TSHashObject<TIMESTAMPSTRUCT, HASHKEY_NONE> {
  LISTDECL(GAMETIMECBSTRUCT, callbackList);

  ~TIMESTAMPSTRUCT() {
  }
};

class CGameTime : public WowTime {
 public:
  DWORD m_lastTick;

  CGameTime();

  void              Destroy();
  int               GetTimeBias() const;
  int               GetDateBias() const;
  void              SetTimeDateBias(int timeBias, int dateBias, bool update);
  void              GameTimeSetTime(const WowTime &time);
  void              GameTimeUpdate(float elapsedSeconds);
  void              GameTimeSync(bool reset);
  void              GameTimeSync(const WowTime &time, bool reset);
  float             GameTimeSetMinutesPerSecond(float minutesPerSecond);
  float             GameTimeGetMinutesPerSecond();
  bool              IsDayTime();
  bool              IsNightTime();
  HGAMETIMECALLBACK GameTimeRegisterCallback(const WowTime &time, void(__stdcall *callback)(const WowTime &, LPVOID), LPVOID user);
  void              GameTimeUnregisterCallback(HGAMETIMECALLBACK callbackHandle);
  float             GameTimeGetDayProgression();
  UINT              MinutesSinceBoot();

 private:
  int                                        m_timeBias;
  int                                        m_dateBias;
  UINT                                       m_gameMinutesElapsed;
  float                                      m_gameMinutesPerRealSecond;
  float                                      m_gameMinutesThisTick;
  UINT                                       m_timeDifferential;
  UINT                                       m_lastTickMinute;
  float                                      m_dayProgression;
  TSHashTable<TIMESTAMPSTRUCT, HASHKEY_NONE> m_callbackLists;

  void TickMinute();
  void PerformCallbacks(int minutes);
};

extern CGameTime g_clientGameTime;

BOOL ClientGameTimeTickHandler(LPCVOID data, LPVOID);
void ClientInitializeGameTime();
void ClientDestroyGameTime();
void SetGameTimeForcedChangeCallback(int set, void (*callback)(UINT oldTime, UINT newTime));

#endif
