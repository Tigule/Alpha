#include "Game/GameTime.h"

#include "DayNight.h"

#include <Base/CDataStore.h>

#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "WorldClient/World.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

CGameTime g_clientGameTime;
static void(*s_forcedChangeCallbacks[4])(unsigned int oldTime, unsigned int newTime);

static void UpdateTime() {
  DNInfo *dnInfo = DayNightGetInfo();

  dnInfo->time = g_clientGameTime.GetHourAndMinutes();
  dnInfo->dayProgression = g_clientGameTime.GameTimeGetDayProgression();
  dnInfo->day = static_cast<float>(g_clientGameTime.GetDaysSinceEpoch());
  CWorld::UpdateDayNight(1, 0);

  for (unsigned int index = 0; index < 4; ++index) {
    if (s_forcedChangeCallbacks[index]) {
      s_forcedChangeCallbacks[index](g_clientGameTime.m_hour, g_clientGameTime.m_minute);
    }
  }

  char buffer[256];
  char string[128];
  SStrPrintf(buffer, sizeof(buffer), "Time set to %s", WowTime::WowGetTimeString(&g_clientGameTime, string, sizeof(string)));
  ConsoleWrite(buffer, DEFAULT_COLOR);
}

int CCommand_ShowLocalGameTime(const char *, const char *) {
  char buffer[128];

  SStrPrintf(buffer, sizeof(buffer), "Current game time is %02d:%02d", g_clientGameTime.m_hour, g_clientGameTime.m_minute);
  ConsoleWrite(buffer, DEFAULT_COLOR);
  return 1;
}

int CCommand_ShowServerGameTime(const char *, const char *) {
  CDataStore message;

  message.Put(CMSG_SERVERTIME);
  message.Finalize();
  ClientServices_Send(&message);
  return 1;
}

int CCommand_GameTime(const char *__formal, const char *time) {
  unsigned int hour = SStrToInt(time);
  const char  *minuteText = SStrChr(time, ' ');
  unsigned int minute;

  if (!minuteText) {
    minuteText = SStrChr(time, ':');
  }
  if (minuteText) {
    minute = SStrToInt(minuteText + 1);
  } else {
    minute = 0;
  }

  if (hour >= 24) {
    hour = 23;
  }
  if (minute >= 60) {
    minute = 59;
  }

  WowTime newTime = g_clientGameTime;
  newTime.m_hour = hour;
  newTime.m_minute = minute;

  CDataStore message;
  message.Put(CMSG_GAMETIME_SET);

  WowTime::WowEncodeTime(hour, &newTime);
  message.Put(hour);
  message.Finalize();
  ClientServices_Send(&message);
  return 1;
}

int CCommand_LocalTime(const char *__formal, const char *time) {
  unsigned int hour = SStrToInt(time);
  const char  *minuteText = SStrChr(time, ' ');
  unsigned int minute;

  if (!minuteText) {
    minuteText = SStrChr(time, ':');
  }
  minute = minuteText ? SStrToInt(minuteText + 1) : 0;
  if (hour >= 24) {
    hour = 23;
  }
  if (minute >= 60) {
    minute = 59;
  }

  WowTime newTime = g_clientGameTime;
  newTime.m_hour = hour;
  newTime.m_minute = minute;
  g_clientGameTime.SetTimeDateBias(0, 0, false);
  g_clientGameTime.SetTimeDateBias(newTime.GetHourAndMinutes() - g_clientGameTime.GetHourAndMinutes(), 0, true);
  UpdateTime();
  return 1;
}

int CCommand_SpawnTime(const char *__formal, const char *time) {
  unsigned int hour = SStrToInt(time);
  const char  *minuteText = SStrChr(time, ' ');
  unsigned int minute;

  if (!minuteText) {
    minuteText = SStrChr(time, ':');
  }
  if (minuteText) {
    minute = SStrToInt(minuteText + 1);
  } else {
    minute = 0;
  }

  CDataStore msg;
  msg.Put(CMSG_ADVANCE_SPAWN_TIME);
  msg.Put(hour * 60 + minute);
  msg.Finalize();
  ClientServices_Send(&msg);
  return 1;
}

int CCommand_GameSpeed(const char *__formal, const char *speed) {
  CDataStore message;

  message.Put(CMSG_GAMESPEED_SET);
  message.Put(SStrToFloat(speed));
  message.Finalize();
  ClientServices_Send(&message);
  return 1;
}

int ReceiveNewGameSpeed(void *, NETMESSAGE msgId, unsigned long, CDataStore *msg) {
  float newSpeed;
  msg->Get(newSpeed);

  if (!msg->IsRead()) {
    ConsoleWriteA("Malformed message recieved: Id = %d, Len = %d, Read = %d\n", DEFAULT_COLOR, msgId, msg->Size(), msg->Tell());
    return 0;
  }

  float oldSpeed = g_clientGameTime.GameTimeSetMinutesPerSecond(newSpeed);
  char  buffer[256];
  SStrPrintf(buffer, sizeof(buffer), "Gamespeed set from %.03f to %.03f", oldSpeed, newSpeed);
  ConsoleWrite(buffer, DEFAULT_COLOR);
  return 1;
}

int ReceiveNewTimeSpeed(void *, NETMESSAGE msgId, unsigned long, CDataStore *msg) {
  unsigned int gameTime;
  float        newSpeed;
  msg->Get(gameTime);
  msg->Get(newSpeed);

  if (!msg->IsRead()) {
    ConsoleWriteA("Malformed message recieved: Id = %d, Len = %d, Read = %d\n", DEFAULT_COLOR, msgId, msg->Size(), msg->Tell());
    return 0;
  }

  g_clientGameTime.GameTimeSetTime(WowTime(gameTime));
  UpdateTime();
  float oldSpeed = g_clientGameTime.GameTimeSetMinutesPerSecond(newSpeed);
  char  buffer[256];
  SStrPrintf(buffer, sizeof(buffer), "Gamespeed set from %.03f to %.03f", oldSpeed, newSpeed);
  ConsoleWrite(buffer, DEFAULT_COLOR);
  return 1;
}

int ReceiveGameTimeUpdate(void *, NETMESSAGE msgId, unsigned long, CDataStore *msg) {
  unsigned int gameTime;
  msg->Get(gameTime);

  if (!msg->IsRead()) {
    ConsoleWriteA("Malformed message recieved: Id = %d, Len = %d, Read = %d\n", DEFAULT_COLOR, msgId, msg->Size(), msg->Tell());
    return 0;
  }

  g_clientGameTime.GameTimeSync(WowTime(gameTime), false);
  return 1;
}

int ReceiveServerTime(void *, NETMESSAGE msgId, unsigned long, CDataStore *msg) {
  unsigned int gameTime;
  msg->Get(gameTime);

  if (!msg->IsRead()) {
    ConsoleWriteA("Malformed message recieved: Id = %d, Len = %d, Read = %d\n", DEFAULT_COLOR, msgId, msg->Size(), msg->Tell());
    return 0;
  }

  WowTime time(gameTime);

  char string[128];
  char buffer[256];
  SStrPrintf(buffer, sizeof(buffer), "Server game time is %s", WowTime::WowGetTimeString(&time, string, sizeof(string)));
  ConsoleWrite(buffer, DEFAULT_COLOR);
  return 1;
}

int ReceiveNewGameTime(void *, NETMESSAGE msgId, unsigned long, CDataStore *msg) {
  unsigned int gameTime;
  msg->Get(gameTime);

  if (!msg->IsRead()) {
    ConsoleWriteA("Malformed message recieved: Id = %d, Len = %d, Read = %d\n", DEFAULT_COLOR, msgId, msg->Size(), msg->Tell());
    return 0;
  }

  g_clientGameTime.GameTimeSetTime(WowTime(gameTime));
  UpdateTime();
  return 1;
}

int ClientGameTimeTickHandler(const void *data, void *__formal) {
  FATALASSERT(data);
  g_clientGameTime.GameTimeUpdate(*static_cast<const float *>(data));
  return 1;
}

void ClientInitializeGameTime() {
  ClientServices_SetMessageHandler(SMSG_GAMESPEED_SET, ReceiveNewGameSpeed, 0);
  ClientServices_SetMessageHandler(SMSG_LOGIN_SETTIMESPEED, ReceiveNewTimeSpeed, 0);
  ClientServices_SetMessageHandler(SMSG_GAMETIME_UPDATE, ReceiveGameTimeUpdate, 0);
  ClientServices_SetMessageHandler(SMSG_SERVERTIME, ReceiveServerTime, 0);
  ClientServices_SetMessageHandler(SMSG_GAMETIME_SET, ReceiveNewGameTime, 0);

  ConsoleCommandRegister("dtime", CCommand_ShowLocalGameTime, DEFAULT, 0);
  ConsoleCommandRegister("time", CCommand_ShowServerGameTime, DEFAULT, 0);
  ConsoleCommandRegister("gametime", CCommand_GameTime, DEFAULT, 0);
  ConsoleCommandRegister("localtime", CCommand_LocalTime, DEFAULT, 0);
  ConsoleCommandRegister("gamespeed", CCommand_GameSpeed, DEFAULT, 0);
  ConsoleCommandRegister("spawntime", CCommand_SpawnTime, DEFAULT, 0);

  memset(s_forcedChangeCallbacks, 0, sizeof(s_forcedChangeCallbacks));
}

void ClientDestroyGameTime() {
  ConsoleCommandUnregister("dtime");
  ConsoleCommandUnregister("time");
  ConsoleCommandUnregister("localtime");
  ConsoleCommandUnregister("gametime");
  ConsoleCommandUnregister("gamespeed");
  ConsoleCommandUnregister("spawntime");

  ClientServices_ClearMessageHandler(SMSG_GAMESPEED_SET);
  ClientServices_ClearMessageHandler(SMSG_LOGIN_SETTIMESPEED);
  ClientServices_ClearMessageHandler(SMSG_GAMETIME_UPDATE);
  ClientServices_ClearMessageHandler(SMSG_SERVERTIME);
  ClientServices_ClearMessageHandler(SMSG_GAMETIME_SET);

  g_clientGameTime.Destroy();
}

void SetGameTimeForcedChangeCallback(
    int set, void(*callback)(unsigned int oldTime, unsigned int newTime)
) {
  for (unsigned int index = 0; index < 4; ++index) {
    if ((!s_forcedChangeCallbacks[index] && set) || (s_forcedChangeCallbacks[index] == callback && !set)) {
      s_forcedChangeCallbacks[index] = set ? callback : 0;
      return;
    }
  }

  if (set) {
    FATALASSERT(!"Warning, not enough free callback slots, add to MAX_GAMETIMECHANGECALLBACKS!");
  }
}
